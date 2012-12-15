#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include "common.h"
#include "log.h"
#include "mtrandom.h"
#include "universe.h"
#include "constellation.h"
#include "system.h"
#include "star.h"
#include "ptrlist.h"
#include "stringtree.h"

int loadconstellations()
{
	struct config *conf;
	struct list_head conf_root = LIST_HEAD_INIT(conf_root);
	unsigned int ncons = 0;

	if (parse_config_file("data/constellations", &conf_root)) {
		destroy_config(&conf_root);
		return -1;
	}

	list_for_each_entry(conf, &conf_root, list) {
		if (ncons >= CONSTELLATION_MAXNUM)
			break;

		printf("Adding constellation %s\n", conf->key);
		if (addconstellation(conf->key))
			goto err;

		ncons++;
	}

	destroy_config(&conf_root);
	return 0;

err:
	destroy_config(&conf_root);
	return -1;
}

int addconstellation(char* cname)
{
	unsigned long nums, numc, i;
	char *string;
	struct system *fs, *s;
	struct ptrlist work;
	double phi;
	unsigned long r;

	string = malloc(strlen(cname)+GREEK_LEN+2);
	if (!string)
		return -1;

	ptrlist_init(&work);

	/* Determine number of systems in constellation */
	nums = mtrandom_uint(GREEK_N);
	if (nums == 0)
		nums = 1;

	mprintf("addconstellation: will create %lu systems (universe has %lu so far)\n", nums, ptrlist_len(&univ.systems));

	pthread_rwlock_wrlock(&univ.systemnames_lock);

	fs = NULL;
	for (numc = 0; numc < nums; numc++) {

		/* Create a new system and put it in s */
		s = malloc(sizeof(*s));
		if (!s)
			goto err;
		sprintf(string, "%s %s", greek[numc], cname);
		if (system_create(s, string))
			goto err;

		ptrlist_push(&univ.systems, s);
		st_add_string(&univ.systemnames, s->name, s);

		if (fs == NULL) {
			/* This was the first system generated for this constellation
			   We need to place this at a suitable point in the universe */
			fs = s;
			if (ptrlist_len(&univ.systems) == 1) {
				/* The first constellation always goes in (0, 0) */
				if (system_move(s, 0, 0))
					bug("%s", "Error when placing first system at (0,0)");
			} else {
				/* All others are randomly distributed */
				phi = mtrandom_uint(UINT_MAX) / (double)UINT_MAX*2*M_PI;
				r = 0;
				i = 2;
				while (i > 1) {
					r += mtrandom_ulong(CONSTELLATION_RANDOM_DISTANCE);
					phi += mtrandom_double(CONSTELLATION_PHI_RANDOM);
					if (!system_move(s, POLTOX(phi, r), POLTOY(phi, r)))
						i = get_neighbouring_systems(NULL, s, CONSTELLATION_MIN_DISTANCE);
				}
			}
			ptrlist_push(&work, s);
		} else if (ptrlist_len(&work) == 0) {
			/* This isn't the first system but no systems are left in work
			   Put this close to the first system */
			ptrlist_push(&work, s);
			makeneighbours(fs, s, 0, 0);
		} else {
			/* We have systems in work, put this close to work[0] and add this one to work */
			ptrlist_push(&work, s);
			makeneighbours(ptrlist_entry(&work, 0), s, 0, 0);
			/* Determine if work[0] has enough neighbours, if so remove it */
			if (mtrandom_uint(UINT_MAX) < UINT_MAX/CONSTELLATION_NEIGHBOUR_CHANCE)
				ptrlist_pull(&work);
		}

		mprintf("Created %s (%p) at %ldx%ld\n", s->name, s, s->x, s->y);

	}

	pthread_rwlock_unlock(&univ.systemnames_lock);

	free(string);
	ptrlist_free(&work);

	return 0;

err:
	pthread_rwlock_unlock(&univ.systemnames_lock);
	return -1;
}

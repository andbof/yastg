#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include "common.h"
#include "log.h"
#include "mtrandom.h"
#include "universe.h"
#include "constellation.h"
#include "sector.h"
#include "star.h"
#include "ptrlist.h"
#include "stringtree.h"

int loadconstellations()
{
	struct config *ctree, *e;
	unsigned int ncons = 0;

	ctree = parseconfig("data/constellations");
	e = ctree->sub;

	if (!e->next)
		die("%s contained no useful data", e->key);

	e = e->next;

	while ((e) && (ncons < CONSTELLATION_MAXNUM)) {
		printf("Adding constellation %s\n", e->key);
		if (addconstellation(e->key))
			goto err;

		e = e->next;
		ncons++;
	}

	destroyctree(ctree);
	return 0;

err:
	destroyctree(ctree);
	return -1;
}

int addconstellation(char* cname)
{
	unsigned long nums, numc, i;
	char *string;
	struct sector *fs, *s;
	struct ptrlist work;
	double phi;
	unsigned long r;

	string = malloc(strlen(cname)+GREEK_LEN+2);
	if (!string)
		return -1;

	ptrlist_init(&work);

	/* Determine number of sectors in constellation */
	nums = mtrandom_uint(GREEK_N);
	if (nums == 0)
		nums = 1;

	mprintf("addconstellation: will create %lu sectors (universe has %lu so far)\n", nums, ptrlist_len(&univ.sectors));

	pthread_rwlock_wrlock(&univ.sectornames_lock);

	fs = NULL;
	for (numc = 0; numc < nums; numc++) {

		/* Create a new sector and put it in s */
		s = malloc(sizeof(*s));
		if (!s)
			goto err;
		sprintf(string, "%s %s", greek[numc], cname);
		if (sector_create(s, string))
			goto err;

		ptrlist_push(&univ.sectors, s);
		st_add_string(&univ.sectornames, s->name, s);

		if (fs == NULL) {
			/* This was the first sector generated for this constellation
			   We need to place this at a suitable point in the universe */
			fs = s;
			if (ptrlist_len(&univ.sectors) == 1) {
				/* The first constellation always goes in (0, 0) */
				s->x = 0;
				s->y = 0;
			} else {
				/* All others are randomly distributed */
				phi = mtrandom_uint(UINT_MAX) / (double)UINT_MAX*2*M_PI;
				r = 0;
				do {
					r += mtrandom_ulong(CONSTELLATION_RANDOM_DISTANCE);
					phi += mtrandom_double(CONSTELLATION_PHI_RANDOM);
					s->x = POLTOX(phi, r);
					s->y = POLTOY(phi, r);
					i = countneighbours(s, CONSTELLATION_MIN_DISTANCE);
				} while (i > 0);
			}
			ptrlist_push(&work, s);
		} else if (ptrlist_len(&work) == 0) {
			/* This isn't the first sector but no sectors are left in work
			   Put this close to the first sector */
			ptrlist_push(&work, s);
			makeneighbours(fs, s, 0, 0);
		} else {
			/* We have sectors in work, put this close to work[0] and add this one to work */
			ptrlist_push(&work, s);
			makeneighbours(ptrlist_entry(&work, 0), s, 0, 0);
			/* Determine if work[0] has enough neighbours, if so remove it */
			if (mtrandom_uint(UINT_MAX) < UINT_MAX/CONSTELLATION_NEIGHBOUR_CHANCE)
				ptrlist_pull(&work);
		}

		mprintf("Created %s (%p) at %ldx%ld\n", s->name, s, s->x, s->y);

	}

	pthread_rwlock_unlock(&univ.sectornames_lock);

	free(string);
	ptrlist_free(&work);

	return 0;

err:
	pthread_rwlock_unlock(&univ.sectornames_lock);
	return -1;
}

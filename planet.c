#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <pthread.h>
#include "base.h"
#include "common.h"
#include "stringtree.h"
#include "log.h"
#include "mtrandom.h"
#include "parseconfig.h"
#include "planet.h"
#include "planet_type.h"
#include "ptrlist.h"
#include "system.h"
#include "universe.h"

static void planet_init(struct planet *p)
{
	memset(p, 0, sizeof(*p));

	ptrlist_init(&p->bases);
	ptrlist_init(&p->stations);
	ptrlist_init(&p->moons);
	INIT_LIST_HEAD(&p->list);
}

void planet_free(struct planet *p)
{
	assert(p != NULL);
	struct base *b;
	struct planet *m;
	struct list_head *lh;
	ptrlist_for_each_entry(b, &p->bases, lh)
		base_free(b);
	ptrlist_free(&p->bases);
	ptrlist_for_each_entry(b, &p->stations, lh)
		base_free(b);
	ptrlist_free(&p->stations);
	ptrlist_for_each_entry(m, &p->moons, lh)
		planet_free(m);
	ptrlist_free(&p->moons);
	if (p->name) {
		st_rm_string(&univ.planetnames, p->name);
		free(p->name);
	}
	if (p->gname) {
		st_rm_string(&univ.planetnames, p->gname);
		free(p->gname);
	}
	free(p);
}

#define PLANET_MUL_ODDS 2
#define PLANET_MUL_MAX 10 
static int planet_gennum()
{
	int num = 1;
	unsigned int ran;
	while ((ran = mtrandom_uint(UINT_MAX)) < UINT_MAX/PLANET_MUL_ODDS)
		num++;
	if (num > PLANET_MUL_MAX)
		num = PLANET_MUL_MAX;
	return num;
}

#define PLANET_HOT_ODDS 3
#define PLANET_ECO_ODDS 5
#define PLANET_COLD_ODDS 10
#define PLANET_DIST_MAX (100 * GM_PER_AU)	/* Based on sol system */
static void planet_genesis(struct planet *planet, struct system *system)
{
	planet->system = system;
	unsigned int tmp = mtrandom_uint(PLANET_HOT_ODDS + PLANET_ECO_ODDS + PLANET_COLD_ODDS);
	enum planet_zone zone;
	if (tmp <= PLANET_HOT_ODDS)
		zone = HOT;
	else if (tmp > PLANET_HOT_ODDS && tmp <= PLANET_HOT_ODDS + PLANET_ECO_ODDS)
		zone = ECO;
	else
		zone = COLD;
	
	unsigned int i, j;
	do {
		i = mtrandom_uint(list_len(&univ.planet_types));
		j = 0;
		list_for_each_entry(planet->type, &univ.planet_types, list) {
			if (j++ == i)
				break;
		}
	} while (planet->type->zones[zone] == 0);
	
	planet->dia = mtrandom_uint(planet->type->maxdia - planet->type->mindia) + planet->type->mindia;
	planet->life = mtrandom_uint(planet->type->maxlife - planet->type->minlife) + planet->type->minlife;

	switch (zone) {
	case HOT:
		planet->dist = mtrandom_uint(system->hablow);
		break;
	case ECO:
		planet->dist = mtrandom_uint(system->habhigh - system->hablow) + system->hablow;
		break;
	case COLD:
		/* FIXME: Some kind of logarithmic function ... ? */
		planet->dist = mtrandom_uint(PLANET_DIST_MAX - system->habhigh) + system->habhigh;
		break;
	default:
		bug("%s", "illegal execution point");
	}

	base_populate_planet(planet);
}

int planet_populate_system(struct system* system)
{
	struct planet *p;
	int num = planet_gennum();

	pthread_rwlock_wrlock(&univ.planetnames_lock);

	for (int i = 0; i < num; i++) {
		p = malloc(sizeof(*p));
		if (!p)
			return -1;

		planet_init(p);
		planet_genesis(p, system);

		p->name = malloc(strlen(system->name) + ROMAN_LEN + 2);
		if (!p->name) {
			free(p);
			return -1;
		}

		sprintf(p->name, "%s %s", system->name, roman[i]);	/* FIXME: Wait with naming until all are made, sort on mean distance from sun. */
		ptrlist_push(&system->planets, p);
		st_add_string(&univ.planetnames, p->name, p);
		if (p->gname)
			st_add_string(&univ.planetnames, p->gname, p);
	}

	pthread_rwlock_unlock(&univ.planetnames_lock);

	return 0;
}

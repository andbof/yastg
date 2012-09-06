#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include "base.h"
#include "common.h"
#include "data.h"
#include "htable.h"
#include "log.h"
#include "mtrandom.h"
#include "parseconfig.h"
#include "planet.h"
#include "ptrlist.h"
#include "sector.h"
#include "universe.h"

struct planet* initplanet()
{
	struct planet *p;
	MALLOC_DIE(p, sizeof(*p));
	memset(p, 0, sizeof(*p));
	p->bases = ptrlist_init();
	p->stations = ptrlist_init();
	p->moons = ptrlist_init();
	INIT_LIST_HEAD(&p->list);
	return p;
}

struct planet* loadplanet(struct configtree *ctree)
{
	struct base *b;
	struct planet *p = initplanet();
	while (ctree) {
		if (strcmp(ctree->key, "NAME") == 0) {
			p->name = strdup(ctree->data);
		} else if (strcmp(ctree->key, "TYPE") == 0) {
			p->type = ctree->data[0];
		} else if (strcmp(ctree->key, "BASE") == 0) {
			b = loadbase(ctree->sub);
			ptrlist_push(p->bases, b);
		}
		ctree = ctree->next;
	}
	return p;
}

void planet_free(struct planet *p)
{
	assert(p != NULL);
	struct base *b;
	struct planet *m;
	struct list_head *lh;
	ptrlist_for_each_entry(b, p->bases, lh)
		base_free(b);
	ptrlist_free(p->bases);
	ptrlist_for_each_entry(b, p->stations, lh)
		base_free(b);
	ptrlist_free(p->stations);
	ptrlist_for_each_entry(m, p->moons, lh)
		planet_free(m);
	ptrlist_free(p->moons);
	if (p->name) {
		htable_rm(univ->planetnames, p->name);
		free(p->name);
	}
	if (p->gname) {
		htable_rm(univ->planetnames, p->gname);
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
static void planet_genesis(struct planet *planet, struct sector *sector)
{
	planet->sector = sector;
	unsigned int tmp = mtrandom_uint(PLANET_HOT_ODDS + PLANET_ECO_ODDS + PLANET_COLD_ODDS);
	enum planet_zone zone;
	if (tmp <= PLANET_HOT_ODDS)
		zone = HOT;
	else if (tmp > PLANET_HOT_ODDS && tmp <= PLANET_HOT_ODDS + PLANET_ECO_ODDS)
		zone = ECO;
	else
		zone = COLD;
	
	do {
		planet->type = mtrandom_uint(PLANET_TYPE_N);
	} while (planet_types[planet->type].zones[zone] == 0);

	struct planet_type *type = &planet_types[planet->type];
	
	planet->dia = mtrandom_uint(type->maxdia - type->mindia) + type->mindia;
	planet->life = mtrandom_uint(type->maxlife - type->minlife) + type->minlife;

	switch (zone) {
	case HOT:
		planet->dist = mtrandom_uint(sector->hablow);
		break;
	case ECO:
		planet->dist = mtrandom_uint(sector->habhigh - sector->hablow) + sector->hablow;
		break;
	case COLD:
		/* FIXME: Some kind of logarithmic function ... ? */
		planet->dist = mtrandom_uint(PLANET_DIST_MAX - sector->habhigh) + sector->habhigh;
		break;
	default:
		bug("%s", "illegal execution point");
	}

	base_populate_planet(planet);
}

void planet_populate_sector(struct sector* sector)
{
	struct planet *p;
	int num = planet_gennum();

	pthread_rwlock_wrlock(&univ->planetnames->lock);

	for (int i = 0; i < num; i++) {
		p = initplanet();
		planet_genesis(p, sector);
		MALLOC_DIE(p->name, strlen(sector->name) + ROMAN_LEN + 2);
		sprintf(p->name, "%s %s", sector->name, roman[i]);	/* FIXME: Wait with naming until all are made, sort on mean distance from sun. */
		ptrlist_push(sector->planets, p);
		htable_add(univ->planetnames, p->name, p);
		if (p->gname)
			htable_add(univ->planetnames, p->gname, p);
	}

	pthread_rwlock_unlock(&univ->planetnames->lock);
}

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <math.h>
#include "common.h"
#include "log.h"
#include "universe.h"
#include "ptrlist.h"
#include "planet.h"
#include "base.h"
#include "sector.h"
#include "civ.h"
#include "star.h"
#include "constellation.h"
#include "stringtree.h"
#include "mtrandom.h"

/*
 * The universe consists of a number of sectors. These sectors are grouped in constellations,
 * with the constellations having limited access between one another.
 *
 * Example: (. is a sector, -|\/ are valid travel routes)
 * 
 * .-.-.     .-.-.
 *  \  |    /  | |
 *   .-.---.---.-.
 *   |    /
 *   .-.-.
 *   | | |
 *   .-.-.
 *
 */

#define NEIGHBOUR_CHANCE 5		/* The higher the value, the more neighbours a system will have */
#define NEIGHBOUR_DISTANCE_LY (50 * TICK_PER_LY)

struct universe univ;

void universe_free(struct universe *u)
{
	ptrlist_free(&u->sectors);
	st_destroy(&u->sectornames, ST_DONT_FREE_DATA);
	st_destroy(&u->planetnames, ST_DONT_FREE_DATA);
	st_destroy(&u->basenames, ST_DONT_FREE_DATA);
	if (u->name)
		free(u->name);
}

void linksectors(struct sector *s1, struct sector *s2)
{
	ptrlist_push(&s1->links, s2);
	ptrlist_push(&s2->links, s1);
}

int makeneighbours(struct sector *s1, struct sector *s2, unsigned long min, unsigned long max)
{
	unsigned long x, y;

	if (max > min) {
		s2->x = min + mtrandom_ulong(max - min) + s1->x;
		s2->y = min + mtrandom_ulong(max - min) + s1->y;
	} else {
		s2->x = mtrandom_ulong(NEIGHBOUR_DISTANCE_LY) * 2 - NEIGHBOUR_DISTANCE_LY + s1->x;
		s2->y = mtrandom_ulong(NEIGHBOUR_DISTANCE_LY) * 2 - NEIGHBOUR_DISTANCE_LY + s1->y;
	}

	return 0;
	/*
	 * FIXME: Don't place too close to another system.
	 * Perhaps try a limited number of times, then return failure.
	 */
}

/*
 * Returns an array of all neighbouring systems within dist ly
 * FIXME: This really needs a more efficient implementation.
 */
struct ptrlist* getneighbours(struct sector *s, unsigned long dist)
{
	size_t st;
	struct sector *t;
	struct ptrlist *r;
	struct list_head *lh;

	r = malloc(sizeof(*r));
	if (!r)
		return NULL;
	ptrlist_init(r);

	ptrlist_for_each_entry(t, &univ.sectors, lh) {
		if (sector_distance(s, t) < dist)
			ptrlist_push(r, t);
	}

	return r;
}

size_t countneighbours(struct sector *s, unsigned long dist)
{
	size_t st, r = 0;
	struct sector *t;
	struct list_head *lh;

	ptrlist_for_each_entry(t, &univ.sectors, lh) {
		if ((t != s) && (sector_distance(s, t) < dist))
			r++;
	}

	return r;
}

void universe_init(struct universe *u)
{
	time(&u->created);
	u->id = 0;
	u->name = NULL;
	u->numsector = 0;
	ptrlist_init(&u->sectors);
	INIT_LIST_HEAD(&u->sectornames);
	pthread_rwlock_init(&u->sectornames_lock, NULL);
	INIT_LIST_HEAD(&u->planetnames);
	pthread_rwlock_init(&u->planetnames_lock, NULL);
	INIT_LIST_HEAD(&u->basenames);
	pthread_rwlock_init(&u->basenames_lock, NULL);
}

int universe_genesis(struct universe *univ, struct civ *civs)
{
	int i;
	int power = 0;
	struct civ *c;

	list_for_each_entry(c, &civs->list, list)
		power += c->power;

	/*
	 * 1. Decide number of constellations in universe.
	 * 2. For each constellation, create a number of sectors, grouping them together.
	 */
	if (loadconstellations(univ))
		return -1;

	/*
	 * 3. Randomly distribute civilizations
	 * 4. Let civilizations grow and create hyperspace links
	 */
	civ_spawncivs(univ, civs);

	return 0;
}

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <math.h>
#include "defines.h"
#include "log.h"
#include "universe.h"
#include "sarray.h"
#include "ptrarray.h"
#include "array.h"
#include "id.h"
#include "planet.h"
#include "base.h"
#include "sector.h"
#include "civ.h"
#include "star.h"
#include "constellation.h"
#include "stable.h"
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
#define NEIGHBOUR_DISTANCE 50

void universe_free(struct universe *u)
{
	ptrarray_free(u->sectors);
	stable_free(u->sectornames);
	sarray_free(u->srad);
	free(u->srad);
	sarray_free(u->sphi);
	free(u->sphi);
	if (u->name)
		free(u->name);
	free(u);
}

void linksectors(struct sector *s1, struct sector *s2)
{
	ptrarray_push(s1->links, s2);
	ptrarray_push(s2->links, s1);
}

int makeneighbours(struct sector *s1, struct sector *s2, unsigned long min, unsigned long max)
{
	unsigned long x, y;

	if (max > min) {
		x = min + mtrandom_ulong(max - min) + s1->x;
		y = min + mtrandom_ulong(max - min) + s1->y;
	} else {
		x = mtrandom_ulong(NEIGHBOUR_DISTANCE)*2 - NEIGHBOUR_DISTANCE + s1->x;
		y = mtrandom_ulong(NEIGHBOUR_DISTANCE)*2 - NEIGHBOUR_DISTANCE + s1->y;
	}
	sector_move(s2, x, y);

	return 0;
	/*
	 * FIXME: Don't place too close to another system.
	 * Perhaps try a limited number of times, then return failure.
	 */
}

/*
 * Returns an array of all neighbouring systems within dist ly
 * FIXME: This really needs a more efficient implementation, perhaps using u->srad or the like.
 */
struct ptrarray* getneighbours(struct sector *s, unsigned long dist)
{
	size_t st;
	struct sector *t;
	struct ptrarray *r = ptrarray_init(0);

	for (st = 0; st < univ->sectors->elements; st++) {
		t = ptrarray_get(univ->sectors, st);
		if (sector_distance(s, t) < dist)
			ptrarray_push(r, t);
	}

	return r;
}

size_t countneighbours(struct sector *s, unsigned long dist)
{
	size_t st, r = 0;
	struct sector *t;

	for (st = 0; st < univ->sectors->elements; st++) {
		t = ptrarray_get(univ->sectors, st);
		if ((t != s) && (sector_distance(s, t) < dist))
			r++;
	}

	return r;
}

struct universe* universe_create()
{
	struct universe *u;

	MALLOC_DIE(u, sizeof(*u));
	u->id = 0;
	u->name = NULL;
	u->numsector = 0;
	u->sectors = ptrarray_init(0);
	u->sectornames = stable_create();
	u->srad = sarray_init(sizeof(struct ulong_ptr), 0, SARRAY_ALLOW_MULTIPLE, NULL, &sort_ulong);
	u->sphi = sarray_init(sizeof(struct double_ptr), 0, SARRAY_ALLOW_MULTIPLE, NULL, &sort_double);

	return u;
}

void universe_init(struct array *civs)
{
	int i;
	int power = 0;

	for (i = 0; i < civs->elements; i++)
		power += ((struct civ*)array_get(civs, i))->power;

	/*
	 * 1. Decide number of constellations in universe.
	 * 2. For each constellation, create a number of sectors, grouping them together.
	 */
	loadconstellations(univ);

	/*
	 * 3. Randomly distribute civilizations
	 * 4. Let civilizations grow and create hyperspace links
	 */
	civ_spawncivs(univ, civs);
}

struct sector* getsectorbyname(struct universe *u, char *name)
{
	unsigned long l;
	return stable_get(univ->sectornames, name);
}

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
#include "array.h"
#include "id.h"
#include "planet.h"
#include "base.h"
#include "sector.h"
#include "civ.h"
#include "star.h"
#include "constellation.h"
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

#define NEIGHBOUR_CHANCE 5		// The higher the value, the more neighbours a system will have
#define NEIGHBOUR_DISTANCE 50

void universe_free(struct universe *u) {
  sarray_free(u->sectors);
  free(u->sectors);
  sarray_free(u->srad);
  free(u->srad);
  sarray_free(u->sphi);
  free(u->sphi);
  if (u->name)
    free(u->name);
  free(u);
}

void linksectors(struct universe *u, size_t id1, size_t id2) {
  struct sector *s1, *s2;
  if (!(s1 = sarray_getbyid(u->sectors, &id1)))
    bug("Sector %zx to link with %zx not found\n", id1, id2);
  if (!(s2 = sarray_getbyid(u->sectors, &id2)))
    bug("Failed finding %zx to link with %zx\n", id2, id1);
  sarray_add(s1->linkids, &id2);
  sarray_add(s2->linkids, &id1);
}

int makeneighbours(struct universe *u, size_t id1, size_t id2, unsigned long min, unsigned long max) {
  struct sector *s1, *s2;
  unsigned long x, y;
  if (!(s1 = sarray_getbyid(u->sectors, &id1)))
    bug("Sector %zx to get %zx as a neighbour not found\n", id1, id2);
  if (!(s2 = sarray_getbyid(u->sectors, &id2)))
    bug("Failed finding %zx in to place near %zx\n", id2, id1);
  if (max > min) {
    x = min + mtrandom_ulong(max - min) + s1->x;
    y = min + mtrandom_ulong(max - min) + s1->y;
  } else {
    x = mtrandom_ulong(NEIGHBOUR_DISTANCE)*2 - NEIGHBOUR_DISTANCE + s1->x;
    y = mtrandom_ulong(NEIGHBOUR_DISTANCE)*2 - NEIGHBOUR_DISTANCE + s1->y;
  }
  sector_move(u, s2, x, y);
  return 1;
  // FIXME: Placera inte för nära ett annat system
  // försök upp till X gånger, sen returnera 0
}

/*
 * Returns an sarray of all neighbouring systems within dist ly
 * FIXME: This really needs a more efficient implementation, perhaps using u->srad or the like.
 * Perhaps it should also return indices instead of an sarray.
 */
struct sarray* getneighbours(struct universe *u, struct sector *s, unsigned long dist) {
  size_t i;
  struct sector *t;
  struct sarray *result = sarray_init(sizeof(size_t), 0, SARRAY_ENFORCE_UNIQUE, NULL, &sort_id);
  for (i = 0; i < u->sectors->elements; i++) {
    t = (struct sector*)sarray_getbypos(u->sectors, i);
    if ((t->id != s->id) && (sector_distance(t, s) < dist))
      sarray_add(result, &t->id);
  }
  return result;
}

size_t countneighbours(struct universe *u, struct sector *s, unsigned long dist) {
  size_t i, n = 0;
  struct sector *t;
  for (i = 0; i < u->sectors->elements; i++) {
    t = (struct sector*)sarray_getbypos(u->sectors, i);
    if ((t->id != s->id) && (sector_distance(t, s) < dist))
      n++;
  }
  return n;
}

struct universe* createuniverse(struct sarray *civs) {
  int i;
  struct universe *u;
  int power = 0;

  MALLOC_DIE(u, sizeof(*u));
  u->id = 0;
  u->name = NULL;
  u->numsector = 0;
  u->sectors = sarray_init(sizeof(struct sector), 0, SARRAY_ENFORCE_UNIQUE, &sector_free, &sort_id);
  u->srad = sarray_init(sizeof(struct ulong_id), 0, SARRAY_ALLOW_MULTIPLE, NULL, &sort_ulong);
  u->sphi = sarray_init(sizeof(struct double_id), 0, SARRAY_ALLOW_MULTIPLE, NULL, &sort_double);
  for (i = 0; i < civs->elements; i++) {
    power += ((struct civ*)sarray_getbypos(civs, i))->power;
  }
  /*
   * 1. Decide number of constellations in universe.
   * 2. For each constellation, create a number of sectors, grouping them together.
   */
  loadconstellations(u);
  /*
   * 3. Randomly distribute civilizations
   * 4. Let civilizations grow and create hyperspace links
   */
  spawncivs(u, civs);
  return u;

}

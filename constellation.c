#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include "defines.h"
#include "log.h"
#include "mtrandom.h"
#include "universe.h"
#include "constellation.h"
#include "id.h"
#include "sector.h"
#include "star.h"
#include "sarray.h"
#include "array.h"
#include "stable.h"

void loadconstellations(struct universe *u) {
  struct configtree *ctree, *e;
  size_t ncons = 0;
  ctree = parseconfig("data/constellations");
  e = ctree->sub;
  if (!e->next)
    die("%s contained no useful data", e->key);
  e = e->next;
  while ((e) && (ncons < CONSTELLATION_MAXNUM)) {
    printf("Calling addconstellation for %s\n", e->key);
    addconstellation(u, e->key);
    e = e->next;
    ncons++;
  }
  destroyctree(ctree);
}

void addconstellation(struct universe *u, char* cname) {
  size_t nums, numc, fs, i;
  char *string;
  struct sector *s;
  struct array *work = array_init(sizeof(size_t), 0, NULL);
  unsigned long x, y;
  double phi;
  unsigned long r;
  MALLOC_DIE(string, strlen(cname)+GREEK_LEN+2);

  // Determine number of sectors in constellation
  nums = mtrandom_sizet(GREEK_N);
  if (nums == 0)
    nums = 1;
  
  fs = 0;
  for (numc = 0; numc < nums; numc++) {

    // Create a new sector and put it in s
    sprintf(string, "%s %s", greek[numc], cname);
    s = sector_create(string);
    i = s->id;
    sarray_add(u->sectors, s);
    free(s);
    s = sarray_getbyid(u->sectors, &i);
    stable_add(u->sectornames, s->name, s);

    if (fs == 0) {
      // This was the first sector generated for this constellation
      // We need to place this at a suitable point in the universe
      fs = s->id;
      if (!u->sectors->elements) {
	// The first constellation always goes in (0, 0)
	x = 0;
	y = 0;
	sector_move(u, s, x, y);
      } else {
	// All others are randomly distributed
	phi = mtrandom_sizet(SIZE_MAX) / (double)SIZE_MAX*2*M_PI;
	r = 0;
	do {
          r += mtrandom_sizet(CONSTELLATION_RANDOM_DISTANCE);
	  phi += mtrandom_double(CONSTELLATION_PHI_RANDOM);
	  x = POLTOX(phi, r);
	  y = POLTOY(phi, r);
	  sector_move(u, s, x, y);
	  i = countneighbours(u, s, CONSTELLATION_MIN_DISTANCE);
//	  printf("%zu systems within %d ly (phi, rad) = (%f, %lu)\n", i, CONSTELLATION_MIN_DISTANCE, phi, r);
	} while (i > 0);
//        t = (struct sector*)sarray_getbypos(u->sectors, mtrandom_sizet(u->sectors->elements));
//        makeneighbours(u, t->id, s->id, 1000, 3000);
        printf("Defined %s at %ld %ld (id %zx)\n", s->name, s->x, s->y, s->id);
      }
      array_push(work, &(s->id));
    } else if (work->elements == 0) {
      // This isn't the first sector but no sectors are left in work
      // Put this close to the first sector
      array_push(work, &(s->id));
//      t = (struct sector*)sarray_getbyid(u->sectors, &fs);
//      printf("first sector %s is at %ldx%ld\n", t->name, t->x, t->y);
      makeneighbours(u, fs, s->id, 0, 0);
      printf("Defined %s at %ld %ld (id %zx)\n", s->name, s->x, s->y, s->id);
    } else {
      // We have sectors in work, put this close to work[0] and add this one to work
      array_push(work, &(s->id));
//      t = (struct sector*)sarray_getbyid(u->sectors, array_get(work, 0));
//      printf("first work sector %s is at %ldx%ld\n", t->name, t->x, t->y);
      makeneighbours(u, GET_ID(array_get(work, 0)), s->id, 0, 0);
      // Determine if work[0] has enough neighbours, if so remove it
      if ( mtrandom_sizet(SIZE_MAX) - SIZE_MAX/CONSTELLATION_NEIGHBOUR_CHANCE < 0 ) {
	array_rm(work, 0);
      }
      printf("Defined %s at %ld %ld (id %zx)\n", s->name, s->x, s->y, s->id);
    }
      
  }

  free(string);
  array_free(work);
  free(work);
  
}


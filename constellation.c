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
#include "ptrarray.h"
#include "array.h"
#include "stable.h"

void loadconstellations() {
  struct configtree *ctree, *e;
  size_t ncons = 0;
  ctree = parseconfig("data/constellations");
  e = ctree->sub;
  if (!e->next)
    die("%s contained no useful data", e->key);
  e = e->next;
  while ((e) && (ncons < CONSTELLATION_MAXNUM)) {
    printf("Calling addconstellation for %s\n", e->key);
    addconstellation(e->key);
    e = e->next;
    ncons++;
  }
  destroyctree(ctree);
}

void addconstellation(char* cname) {
  size_t nums, numc, i;
  char *string;
  struct sector *fs, *s;
  struct ptrarray *work = ptrarray_init(0);
  unsigned long x, y;
  double phi;
  unsigned long r;
  MALLOC_DIE(string, strlen(cname)+GREEK_LEN+2);

  // Determine number of sectors in constellation
  nums = mtrandom_sizet(GREEK_N);
  if (nums == 0)
    nums = 1;

  mprintf("addconstellation: will create %zu sectors (universe has %zu so far)\n", nums, univ->sectors->elements);
  
  fs = NULL;
  for (numc = 0; numc < nums; numc++) {

    // Create a new sector and put it in s
    sprintf(string, "%s %s", greek[numc], cname);
    s = sector_create(string);
    ptrarray_push(univ->sectors, s);
    stable_add(univ->sectornames, s->name, s);

    if (fs == NULL) {
      // This was the first sector generated for this constellation
      // We need to place this at a suitable point in the universe
      fs = s;
      if (univ->sectors->elements == 1) {
	mprintf("FIRST SECTOR\n");
	// The first constellation always goes in (0, 0)
	x = 0;
	y = 0;
	sector_move(s, x, y);
	mprintf("Sector %p is at %ldx%ld\n", s, x, y);
      } else {
	mprintf("FIRST SECTOR IN CONSTELLATION\n");
	// All others are randomly distributed
	phi = mtrandom_sizet(SIZE_MAX) / (double)SIZE_MAX*2*M_PI;
	r = 0;
	do {
          r += mtrandom_sizet(CONSTELLATION_RANDOM_DISTANCE);
	  phi += mtrandom_double(CONSTELLATION_PHI_RANDOM);
	  x = POLTOX(phi, r);
	  y = POLTOY(phi, r);
	  mprintf("s = %p, x = %ld, y = %ld\n", s, x, y);
	  sector_move(s, x, y);
	  i = countneighbours(s, CONSTELLATION_MIN_DISTANCE);
	} while (i > 0);
        printf("Defined %s at %ld %ld\n", s->name, s->x, s->y);
      }
      ptrarray_push(work, s);
    } else if (work->elements == 0) {
      mprintf("NOTHING LEFT IN WORK\n");
      // This isn't the first sector but no sectors are left in work
      // Put this close to the first sector
      ptrarray_push(work, s);
      makeneighbours(fs, s, 0, 0);
      printf("Defined %s at %ld %ld\n", s->name, s->x, s->y);
    } else {
      mprintf("%zu SECTORS IN WORK\n", work->elements);
      // We have sectors in work, put this close to work[0] and add this one to work
      ptrarray_push(work, s);
      makeneighbours(ptrarray_get(work, 0), s, 0, 0);
      // Determine if work[0] has enough neighbours, if so remove it
      if ( mtrandom_sizet(SIZE_MAX) - SIZE_MAX/CONSTELLATION_NEIGHBOUR_CHANCE < 0 ) {
	ptrarray_rm(work, 0);
      }
      printf("Defined %s at %ld %ld\n", s->name, s->x, s->y);
    }

    mprintf("created %s\n", s->name);
      
  }

  free(string);
  ptrarray_free(work);
  
}


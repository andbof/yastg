#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include "defines.h"
#include "mtrandom.h"
#include "civ.h"
#include "sector.h"
#include "sarray.h"
#include "array.h"
#include "parseconfig.h"
#include "id.h"
#include "universe.h"

struct civ* loadciv(struct configtree *ctree) {
  struct civ *c;
  MALLOC_DIE(c, sizeof(*c));
  c->id = gen_id();
  struct sector *s;
  char *st;
  c->presectors = array_init(sizeof(struct sector), 0, &sector_free);
  c->availnames = array_init(sizeof(char*), 0, &free);
  c->sectors = sarray_init(sizeof(size_t), 0, SARRAY_ENFORCE_UNIQUE, NULL, &sort_id);
  ctree=ctree->sub;
  while (ctree) {
    if (strcmp(ctree->key, "NAME") == 0) {
      c->name = strdup(ctree->data);
    } else if (strcmp(ctree->key, "HOME") == 0) {
    } else if (strcmp(ctree->key, "POWER") == 0) {
      sscanf(ctree->data, "%d", &c->power);
    } else if (strcmp(ctree->key, "SECTOR") == 0) {
      s = loadsector(ctree->sub);
      array_push(c->presectors, s);
      free(s);	// We don't call free_sector here since free_sector will delete all contents associated with s (and we need to reference them in c->sectors)
    } else if (strcmp(ctree->key, "SNAME") == 0) {
      st = strdup(ctree->data);
      array_push(c->availnames, &st); // FIXME: Will this leak memory?
    }
    ctree = ctree->next;
  }
  return c;
}

struct sarray* loadcivs() {
  DIR *dirp;
  struct dirent *de;
  struct configtree *ctree;
  struct sarray *a = sarray_init(sizeof(struct civ), 0, SARRAY_ENFORCE_UNIQUE, &civ_free, &sort_id);
  struct civ *cv;
  char* path = NULL;
  int pathlen = 0;
  if (!(dirp = opendir("civs"))) die("%s", "opendir() on civs failed");
  while ((de = readdir(dirp)) != NULL) {
    if (de->d_name[0] != '.') {
      if ((int)strlen(de->d_name) > pathlen-6) {
	if (path != NULL) free(path);
	pathlen = strlen(de->d_name)+6;
	path = malloc(pathlen);
      }
      sprintf(path, "%s/%s", "civs", de->d_name);
      ctree = parseconfig(path);
      cv = loadciv(ctree);
      destroyctree(ctree);
      sarray_add(a, cv);
      free(cv);		// All data, including pointers to others objects, have been copied. We need to free cv now.
    }
  }
  if (path != NULL) free(path);
  closedir(dirp);
  return a;
}

void civ_free(void *ptr) {
  struct civ* c = ptr;
  sarray_free(c->sectors);
  array_free(c->presectors);
  free(c->sectors);
  free(c->name);
}

#define UNIVERSE_CIV_FRAC 0.4
#define UNIVERSE_MIN_INTERCIV_DISTANCE 100

void spawncivs(struct universe *u, struct sarray *civs) {
  size_t i, j, k, nhab, chab, tpow;
  struct sector *s;
  struct sarray *neigh;
  struct civ *c;
  nhab = u->sectors->elements * UNIVERSE_CIV_FRAC;
  // Calculate total civilization power (we need this to be able
  // to get their relative values)
  tpow = 0;
  for (i = 0; i < civs->elements; i++) {
    tpow += ((struct civ*)sarray_getbypos(civs, i))->power;
  }
  // First, spawn all civilizations in separate home systems
  for (i = 0; i < civs->elements; i++) {
    c = sarray_getbypos(civs, i);
    do {
      s = sarray_getbypos(u->sectors, mtrandom_sizet(u->sectors->elements));
      k = 1;
      if (!s->owner) {
	neigh = getneighbours(u, s, UNIVERSE_MIN_INTERCIV_DISTANCE);
	for (j = 0; j < neigh->elements; j++) {
	  s = sarray_getbyid(u->sectors, &GET_ID(sarray_getbypos(neigh, j)));
	  k = s->owner;
	  if (k)
	    break;
	}
	sarray_free(neigh);
      } else {
	k = 0;
      }
    } while (s->owner != 0);
    printf("Chose %zx (%s) as home system for %s\n", s->id, s->name, c->name);
    s->owner = c->id;
    c->home = s->id;
    sarray_add(c->sectors, &s->id);
  }
  chab = civs->elements;
  while (chab < nhab) {
    // Grow civilizations
    for (i = 0; i < civs->elements; i++) {
      c = sarray_getbypos(civs, i);
      if (mtrandom_sizet(tpow) < c->power) {
	growciv(u, c);
	chab++;
      }
    }
  }

  printf("Civilization stats:\n");
  for (i = 0; i < civs->elements; i++) {
    c = sarray_getbypos(civs, i);
    printf("  %s has %zu sectors (%.2f%%) with power %u\n", c->name, c->sectors->elements, (float)c->sectors->elements/chab*100, c->power);
  }
  printf("%zu sectors of %zu are inhabited (%.2f%%)\n", chab, u->sectors->elements, (float)chab/u->sectors->elements*100);
}

#define CIV_GROW_MIN 10
#define CIV_GROW_STEP 10

void growciv(struct universe *u, struct civ *c) {
  struct sector *s;
  struct sarray *neigh;
  size_t i;
  size_t *ptr;
  unsigned long rad = CIV_GROW_MIN;
  ptr = sarray_getbypos(c->sectors, mtrandom_sizet(c->sectors->elements));
  do {
    s = NULL;
    neigh = getneighbours(u, sarray_getbyid(u->sectors, ptr), rad);
//    printf("Found %zu neighbors\n", neigh->elements);
    for (i = 0; i < neigh->elements; i++) {
      s = sarray_getbyid(u->sectors, &GET_ID(sarray_getbypos(neigh, i)));
      if (!s->owner) {
//	printf("Found system %s owned by no one!\n", s->name);
	break;
      } else {
//	printf("System %s is already owned by %zx\n", s->name, s->owner);
      }
    }
    if ((s == NULL) || (s->owner)) {
      rad += CIV_GROW_STEP;
 //     printf("radial search distance is now %lu\n", rad);
    }
  } while ((s == NULL) || (s->owner));
  printf("Growing civ %s to system %zx (%s) at %ld %ld\n", c->name, s->id, s->name, s->x, s->y);
  s->owner = c->id;
  // Link sectors *before* adding them to c->sectors (as sarray_add might
  // move ptr if a realloc occurs)
  linksectors(u, s->id, *ptr);
  sarray_add(c->sectors, &s->id);
}

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include "defines.h"
#include "log.h"
#include "sector.h"
#include "planet.h"
#include "base.h"
#include "sarray.h"
#include "parseconfig.h"
#include "id.h"
#include "mtrandom.h"

struct planet* initplanet() {
  struct planet *p;
  MALLOC_DIE(p, sizeof(*p));
  memset(p, 0, sizeof(*p));
  p->id = gen_id();
  p->bases = sarray_init(0, SARRAY_ENFORCE_UNIQUE, &base_free, &sort_id);
  p->moons = sarray_init(0, SARRAY_ENFORCE_UNIQUE, &planet_free, &sort_id);
  return p;
}

struct planet* loadplanet(struct configtree *ctree) {
  struct base *b;
  struct planet *p = initplanet();
  while (ctree) {
    if (strcmp(ctree->key, "NAME") == 0) {
      p->name = strdup(ctree->data);
    } else if (strcmp(ctree->key, "TYPE") == 0) {
      p->type = ctree->data[0];
    } else if (strcmp(ctree->key, "BASE") == 0) {
      b = loadbase(ctree->sub);
      sarray_add(p->bases, b);
    } else if (strcmp(ctree->key, "ID") == 0) {
      rm_id(p->id);
      sscanf(ctree->data, "%zu", &(p->id));
      insert_id(p->id);
    }
    ctree = ctree->next;
  }
  return p;
}

void planet_free(void *ptr) {
  struct planet *p = ptr;
  sarray_free(p->bases);
  free(p->bases);
  sarray_free(p->moons);
  free(p->moons);
  free(p->name);
}

#define PLANET_ODDS 7
#define PLANET_NUM_MAX 11

struct planet* createplanet(struct sector* s) {
  struct planet *p;
  MALLOC_DIE(p, sizeof(*p));
  return p;
}

struct sarray* createplanets(struct sector* s) {
  int num = 0;
  struct planet *p;
  struct sarray* planets = sarray_init(0, SARRAY_ENFORCE_UNIQUE, &planet_free, &sort_id); 
  while ((mtrandom_sizet(SIZE_MAX) - SIZE_MAX/PLANET_ODDS < 0) && (num < PLANET_NUM_MAX)) {
    p = createplanet(s);
    sarray_add(planets, p);
  }
  return planets;
}


#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "defines.h"
#include "sector.h"
#include "planet.h"
#include "base.h"
#include "sarray.h"
#include "parseconfig.h"
#include "id.h"

struct planet* initplanet() {
  struct planet *p;
  MALLOC_DIE(p, sizeof(*p));
  memset(p, 0, sizeof(*p));
  p->id = gen_id();
  p->bases = sarray_init(sizeof(struct base), 0, SARRAY_ENFORCE_UNIQUE, &base_free, &sort_id);
  p->moons = sarray_init(sizeof(struct planet), 0, SARRAY_ENFORCE_UNIQUE, &planet_free, &sort_id);
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
      free(b);
    } else if (strcmp(ctree->key, "ID") == 0) {
      rm_id(p->id);
      sscanf(ctree->data, "%zu", &(p->id));
      insert_id(p->id);
    }
    ctree = ctree->next;
  }
  return p;
}

void planet_free(struct planet *p) {
  sarray_free(p->bases);
  free(p->bases);
  sarray_free(p->moons);
  free(p->moons);
  free(p->name);
}

#define PLANET_ODDS 7
#define PLANET_NUM_MAX 11

struct planet* createplanet() {
  struct planet *p;
  MALLOC_DIE(p, sizeof(*p));
  return p;
}

struct sarray* createplanets(struct sector* s) {
  int num = 0;
  int i;
  struct planet *p;
  struct sarray* planets = sarray_init(sizeof(struct planet), 0, SARRAY_ENFORCE_UNIQUE, &planet_free, &sort_id); 
  while (random() - INT_MAX/PLANET_ODDS < 0)
    num++;
  if (num > PLANET_NUM_MAX)
    num = PLANET_NUM_MAX;
  for (i = 0; i < num; i++) {
    p = createplanet();
    sarray_add(planets, p);
    free(p);
  }
  return planets;
}


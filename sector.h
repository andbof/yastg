#ifndef HAS_SECTOR_H
#define HAS_SECTOR_H

#include "planet.h"
#include "base.h"
#include "civ.h"

struct universe;

struct sector {
  size_t id;
  char* name;
  size_t owner;
  char* gname;
  long x, y;
  unsigned long r;
  double phi;
  int hab;
  unsigned long hablow, habhigh, snowline;
  struct sarray *stars;
  struct sarray *planets;
  struct sarray *bases;
  struct sarray *linkids;
};

struct sector* sector_init();
struct sector* sector_load(struct configtree *ctree);
struct sector* sector_create(char *name);

void sector_free(void *ptr);

void sector_move(struct universe *u, struct sector *s, long x, long y);

unsigned long sector_distance(struct sector *a, struct sector *b);

#endif

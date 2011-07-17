#ifndef HAS_SECTOR_H
#define HAS_SECTOR_H

struct universe;
struct ptrarray;
struct configtree;

struct sector {
  char *name;
  struct civ *owner;
  char *gname;
  long x, y;
  unsigned long r;
  double phi;
  int hab;
  unsigned long hablow, habhigh, snowline;
  struct ptrarray *stars;
  struct ptrarray *planets;
  struct ptrarray *bases;
  struct ptrarray *links;
};

struct sector* sector_init();
struct sector* sector_load(struct configtree *ctree);
struct sector* sector_create(char *name);
void sector_free(void *ptr);

void sector_move(struct sector *s, long x, long y);

unsigned long sector_distance(struct sector *a, struct sector *b);

#endif

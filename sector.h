#ifndef _HAS_SECTOR_H
#define _HAS_SECTOR_H

#include "list.h"
#include "ptrlist.h"

struct universe;
struct ptrlist;
struct config;

struct sector {
	char *name;
	struct civ *owner;
	char *gname;
	long x, y;
	unsigned long r;
	double phi;
	int hab;
	unsigned int hablow, habhigh;
	struct ptrlist stars;
	struct ptrlist planets;
	struct ptrlist bases;
	struct ptrlist links;
	struct list_head list;
};

struct sector* sector_init();
struct sector* sector_load(struct config *ctree);
struct sector* sector_create(char *name);
void sector_free(void *ptr);

void sector_move(struct sector *s, long x, long y);

unsigned long sector_distance(struct sector *a, struct sector *b);

#endif

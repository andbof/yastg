#ifndef _HAS_SECTOR_H
#define _HAS_SECTOR_H

#include "list.h"
#include "rbtree.h"
#include "ptrlist.h"

struct universe;
struct ptrlist;
struct config;

struct sector {
	char *name;
	struct civ *owner;
	char *gname;
	long x, y;
	struct rb_node x_rbtree;
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

void sector_init(struct sector *s);
struct sector* sector_load(struct config *ctree);
int sector_create(struct sector *s, char *name);
void sector_free(struct sector *s);

unsigned long sector_distance(const struct sector * const a, const struct sector * const b);

#endif

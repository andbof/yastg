#ifndef _HAS_SYSTEM_H
#define _HAS_SYSTEM_H

#include "list.h"
#include "rbtree.h"
#include "ptrlist.h"

struct universe;
struct ptrlist;
struct config;

struct system {
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

void system_init(struct system *s);
struct system* system_load(struct config *ctree);
int system_create(struct system *s, char *name);
void system_free(struct system *s);

unsigned long system_distance(const struct system * const a, const struct system * const b);

#endif

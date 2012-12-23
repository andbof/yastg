#ifndef _HAS_BASE_H
#define _HAS_BASE_H

#include "ptrlist.h"
#include "list.h"
#include "parseconfig.h"
#include "base_type.h"

struct base_item {
	struct item *item;
	long max;
	long amount;
	long daily_change;
	struct list_head list;
};

struct base {
	char *name;
	struct base_type *type;
	int docks;
	struct planet *planet;
	struct system *system;
	struct list_head items;
	pthread_rwlock_t items_lock;
	struct list_head item_names;
	struct ptrlist players;
};

void base_populate_planet(struct planet* planet);

void base_free(struct base *b);

#endif

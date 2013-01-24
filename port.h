#ifndef _HAS_BASE_H
#define _HAS_BASE_H

#include "ptrlist.h"
#include "list.h"
#include "parseconfig.h"
#include "port_type.h"

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
	struct list_head list;
};

void base_populate_planet(struct planet* planet);

void base_free(struct base *b);

#endif
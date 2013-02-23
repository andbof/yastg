#ifndef _HAS_PORT_H
#define _HAS_PORT_H

#include <pthread.h>
#include "list.h"
#include "parseconfig.h"
#include "planet.h"
#include "port_type.h"
#include "ptrlist.h"

struct port {
	char *name;
	struct port_type *type;
	int docks;
	struct planet *planet;
	struct system *system;
	struct list_head items;
	pthread_rwlock_t items_lock;
	struct list_head item_names;
	struct ptrlist players;
	struct list_head list;
};

void port_populate_planet(struct planet* planet);

void port_free(struct port *b);

#endif

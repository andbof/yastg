#ifndef _HAS_PLANET_H
#define _HAS_PLANET_H

#include "list.h"
#include "ptrlist.h"
#include "system.h"

struct planet {
	char *name;
	char *gname;
	struct planet_type *type;
	unsigned int dia;		/* In hundreds of kilometres */
	unsigned int dist;		/* In gigametres */
	unsigned int life;
	struct ptrlist ports;
	struct ptrlist stations;
	struct ptrlist moons;
	struct ptrlist players;
	struct system *system;
	struct list_head list;
};

void planet_free(struct planet *p);
struct planet* createplanet();
int planet_populate_system(struct system* system);

#endif

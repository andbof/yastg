#ifndef _HAS_PLANET_H
#define _HAS_PLANET_H

#include "system.h"
#include "parseconfig.h"
#include "list.h"
#include "data.h"
#include "ptrlist.h"

struct planet {
	char *name;
	char *gname;
	unsigned int type;
	unsigned int dia;		/* In hundreds of kilometres */
	unsigned int dist;		/* In gigametres */
	unsigned int life;
	struct ptrlist bases;
	struct ptrlist stations;
	struct ptrlist moons;
	struct ptrlist players;
	struct system *system;
	struct list_head list;
};

int planet_load(struct planet *p, struct config *ctree);
void planet_free(struct planet *p);
struct planet* createplanet();
int planet_populate_system(struct system* system);

#endif

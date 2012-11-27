#ifndef _HAS_PLANET_H
#define _HAS_PLANET_H

#include "sector.h"
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
	struct sector *sector;
	struct list_head list;
};

struct planet* loadplanet(struct configtree *ctree);
void planet_free(struct planet *p);
struct planet* createplanet();
void planet_populate_sector(struct sector* s);

#endif

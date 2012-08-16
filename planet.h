#ifndef HAS_PLANET_H
#define HAS_PLANET_H

#include "sector.h"
#include "parseconfig.h"
#include "list.h"

struct sector;

#define PLANET_TYPE_N 17
static char planet_types[PLANET_TYPE_N] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'N', 'O', 'P', 'S', 'Y'
};

struct planet {
	char *name;
	int type;
	struct ptrlist *bases;
	struct ptrlist *moons;
	struct list_head list;
};

struct planet* loadplanet(struct configtree *ctree);

void planet_free(struct planet *p);

struct planet* createplanet();

struct ptrlist* createplanets(struct sector *s);

#endif

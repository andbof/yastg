#ifndef HAS_PLANET_H
#define HAS_PLANET_H

#include "sector.h"
#include "array.h"
#include "parseconfig.h"

struct sector;

#define PLANET_TYPE_N 17
static char planet_types[PLANET_TYPE_N] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'N', 'O', 'P', 'S', 'Y'
};

struct planet {
	char *name;
	int type;
	struct ptrarray *bases;
	struct ptrarray *moons;
};

struct planet* loadplanet(struct configtree *ctree);

void planet_free(void *ptr);

struct planet* createplanet();

struct ptrarray* createplanets(struct sector *s);

#endif

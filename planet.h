#ifndef HAS_PLANET_H
#define HAS_PLANET_H

#include "sector.h"
#include "parseconfig.h"
#include "list.h"

enum planet_zone {
	HOT,
	ECO,
	COLD,
	PLANET_ZONE_NUM
};

enum planet_life {
	TOXIC,
	BARREN,
	DEAD,
	SINGLECELL,
	BACTERIA,
	SIMPLE,
	RESISTANT,
	COMPLEX,
	ANIMAL,
	INTELLIGENT,
	LIFE_NUM
};

struct planet_type {
	char c;
	char *name;
	char *desc;
	char *surface;
	char *atmo;
	int zones[PLANET_ZONE_NUM];
	unsigned int mindia, maxdia;	/* In hundreds of kilometres */
	enum planet_life minlife, maxlife;
};
	
struct planet {
	char *name;
	char *gname;
	unsigned int type;
	unsigned int dia;		/* In hundreds of kilometres */
	unsigned int dist;		/* In gigametres */
	unsigned int life;
	struct ptrlist *bases;
	struct ptrlist *stations;
	struct ptrlist *moons;
	struct sector *sector;
	struct list_head list;
};

struct planet* loadplanet(struct configtree *ctree);
void planet_free(struct planet *p);
struct planet* createplanet();
void planet_populate_sector(struct sector* s);

#endif

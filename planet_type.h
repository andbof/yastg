#ifndef _HAS_PLANET_TYPE_H
#define _HAS_PLANET_TYPE_H

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
	PLANET_LIFE_NUM
};

#define PLANET_LIFE_DESC_LEN 25
extern const char planet_life_desc[PLANET_LIFE_NUM][PLANET_LIFE_DESC_LEN];

#define PLANET_LIFE_NAME_LEN 12
extern char planet_life_names[PLANET_LIFE_NUM][PLANET_LIFE_NAME_LEN];

#define PLANET_ZONE_NAME_LEN 5
extern char planet_zone_names[PLANET_ZONE_NUM][PLANET_ZONE_NAME_LEN];

struct planet_type {
	char c;
	char *name;
	char *desc;
	char *surface;
	char *atmo;
	int zones[PLANET_ZONE_NUM];
	unsigned int mindia, maxdia;	/* In hundreds of kilometres */
	enum planet_life minlife, maxlife;
	struct ptrlist base_types;
	struct list_head list;
};

void planet_type_free(struct planet_type * const type);
int load_all_planets(struct list_head * const root);

#endif
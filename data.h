#ifndef _HAS_DATA_H
#define _HAS_DATA_H

#include "planet.h"
#include "base.h"

#define PLANET_TYPE_N 19
#define PLANET_LIFE_DESC_LEN 25
#define BASE_TYPE_N 1

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
	int bases[BASE_TYPE_N];
};
	
enum base_zone {
	OCEAN,
	SURFACE,
	ORBIT,
	ROGUE,
	BASE_ZONE_NUM
};

struct base_type {
	char *name;
	char *desc;
	int zones[BASE_ZONE_NUM];
};

extern struct planet_type planet_types[PLANET_TYPE_N];

extern const char planet_life_desc[LIFE_NUM][PLANET_LIFE_DESC_LEN];

extern struct base_type base_types[BASE_TYPE_N];

#endif

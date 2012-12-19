#ifndef _HAS_DATA_H
#define _HAS_DATA_H

#include "base.h"

#define BASE_TYPE_N 1

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

extern struct base_type base_types[BASE_TYPE_N];

#endif

#ifndef _HAS_BASE_TYPE_H
#define _HAS_BASE_TYPE_H

#include "ptrlist.h"

enum base_zone {
	OCEAN,
	SURFACE,
	ORBIT,
	ROGUE,
	BASE_ZONE_NUM
};

extern char base_zone_names[BASE_ZONE_NUM][8];

struct base_type_item {
	struct item *item;
	int capacity;
	int daily_change;
	struct ptrlist requires;
	struct list_head list;
};

struct base_type {
	char *name;
	char *desc;
	struct list_head list;
	struct list_head items;
	int zones[BASE_ZONE_NUM];
};

int load_all_bases(struct list_head * const root);
void base_type_free(struct base_type *type);

#endif

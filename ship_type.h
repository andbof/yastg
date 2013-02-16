#ifndef _HAS_SHIP_TYPE_H
#define _HAS_SHIP_TYPE_H

#include "list.h"

struct ship_type {
	char *name;
	char *desc;
	int carry_weight;
	struct list_head list;
};

void ship_type_free(struct ship_type * const type);
int load_all_ships(const char * const file);

#endif

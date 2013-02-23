#ifndef _HAS_SHIP_TYPE_H
#define _HAS_SHIP_TYPE_H

#include "list.h"
#include "universe.h"

struct ship_type {
	char *name;
	char *desc;
	int carry_weight;
	struct list_head list;
};

void ship_type_free(struct ship_type * const type);
int load_ships_from_file(const char * const file, struct universe * const universe);

#endif

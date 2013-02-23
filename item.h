#ifndef _HAS_ITEM_H
#define _HAS_ITEM_H

#include "list.h"
#include "universe.h"

struct item {
	char *name;
	long weight;
	long base_price;
	struct list_head list;
};

int load_items_from_file(const char * const file, struct universe * const universe);
void item_free(struct item * const item);

#endif

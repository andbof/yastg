#ifndef _HAS_ITEM_H
#define _HAS_ITEM_H

#include "list.h"

struct item {
	char *name;
	long weight;
	long base_price;
	struct list_head list;
};

int load_all_items();
void item_free(struct item * const item);

#endif

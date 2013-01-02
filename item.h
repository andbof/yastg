#ifndef _HAS_ITEM_H
#define _HAS_ITEM_H

#include "list.h"

struct item {
	char *name;
	long weight;
	long base_price;
	struct list_head list;
};

int load_all_items(struct list_head * const root, struct list_head * const item_tree);
void item_free(struct item * const item);

#endif

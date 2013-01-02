#ifndef _HAS_CARGO_H
#define _HAS_CARGO_H

#include "list.h"
#include "ptrlist.h"

struct cargo {
	struct item *item;
	long min, max;
	long amount;
	long daily_change;
	long price;
	struct ptrlist requires;
	struct list_head list;
};

void cargo_init(struct cargo *cargo);
void cargo_free(struct cargo *cargo);

#endif

#ifndef HAS_BASE_H
#define HAS_BASE_H

#include "inventory.h"
#include "player.h"
#include "parseconfig.h"

struct base {
	char *name;
	char type;
	int docks;
	void *inventory;
	void *players;
	struct planet *planet;
	struct sector *sector;
};

struct base* loadbase(struct configtree *ctree);

void base_free(void *ptr);

#endif

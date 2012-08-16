#ifndef HAS_PLAYER_H
#define HAS_PLAYER_H

#include "list.h"

struct player {
	char* name;
	struct sector* position;
	struct list_head list;
};

void player_free();

static struct player *alfred;

#endif

#ifndef _HAS_SHIP_H
#define _HAS_SHIP_H

#include "list.h"
#include "ship_type.h"

enum postype {
	NONE,
	SYSTEM,
	BASE,
	PLANET,
	SHIP
};

struct ship {
	char *name;
	struct ship_type *type;
	struct player *owner;
	enum postype postype;
	void *pos;
	struct list_head list;
};

#include "player.h"

void ship_free(struct ship *ship);
int ship_go(struct ship *ship, enum postype postype, void *pos);
int new_ship_to_player(struct ship_type *ship_type, struct player *player);

#endif

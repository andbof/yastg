#ifndef _HAS_SHIP_H
#define _HAS_SHIP_H

#include <pthread.h>
#include "list.h"
#include "cargo.h"
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
	pthread_rwlock_t cargo_lock;
	struct list_head cargo;
	struct list_head cargo_names;
	struct list_head list;
};

#include "player.h"

void ship_free(struct ship *ship);
int ship_go(struct ship *ship, enum postype postype, void *pos);
int new_ship_to_player(struct ship_type *ship_type, struct player *player);

/*
 * Must be called with the appropriate locks (i.e. ship->cargo_lock) held
 */
int move_cargo_to_ship(struct ship * const ship, struct cargo * const cargo, long amount);
int move_cargo_from_ship(struct ship * const ship, struct cargo * const cargo, long amount);

#endif

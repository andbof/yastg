#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "ship.h"
#include "cargo.h"
#include "common.h"
#include "item.h"
#include "stringtrie.h"

static void ship_init(struct ship *ship)
{
	memset(ship, 0, sizeof(*ship));
	INIT_LIST_HEAD(&ship->list);
	INIT_LIST_HEAD(&ship->cargo);
	st_init(&ship->cargo_names);
	pthread_rwlock_init(&ship->cargo_lock, NULL);
}

void ship_free(struct ship *ship)
{
	free(ship->name);
	pthread_rwlock_destroy(&ship->cargo_lock);
	st_destroy(&ship->cargo_names, ST_DONT_FREE_DATA);

	struct cargo *c, *_c;
	list_for_each_entry_safe(c, _c, &ship->cargo, list) {
		list_del(&c->list);
		cargo_free(c);
		free(c);
	}

}

int ship_go(struct ship *ship, enum postype postype, void *pos)
{
	ship->postype = postype;
	ship->pos = pos;

	return 0;
}

int new_ship_to_player(struct ship_type *ship_type, struct player *player)
{
	struct ship *ship;

	ship = malloc(sizeof(*ship));
	if (!ship)
		return -1;
	ship_init(ship);

	ship->name = strdup("Le Fancy Ship With Fancy Name");
	if (!ship->name)
		goto err;

	ship->type = ship_type;
	ship->owner = player;

	list_add(&ship->list, &player->ships);

	return 0;

err:
	ship_free(ship);
	free(ship);
	return -1;
}

static struct cargo* new_cargo_to_ship(struct ship * const ship, const struct cargo * const cargo)
{
	struct cargo *ship_cargo;

	ship_cargo = malloc(sizeof(*ship_cargo));
	if (!ship_cargo)
		return NULL;

	cargo_init(ship_cargo);
	ship_cargo->item = cargo->item;
	ship_cargo->max = LONG_MAX; /* FIXME */
	if (st_add_string(&ship->cargo_names, cargo->item->name, ship_cargo)) {
		cargo_free(ship_cargo);
		free(ship_cargo);
		return NULL;
	}

	list_add(&ship_cargo->list, &ship->cargo);

	return ship_cargo;
}

/*
 * Must be called with the appropriate locks (i.e. ship->cargo_lock) held
 */
int move_cargo_to_ship(struct ship * const ship, struct cargo * const cargo, long amount)
{
	assert(cargo->amount >= 0);

	struct cargo *ship_cargo = st_lookup_string(&ship->cargo_names, cargo->item->name);
	if (!ship_cargo) {
		ship_cargo = new_cargo_to_ship(ship, cargo);
		if (!ship_cargo)
			return -1;
	}
	assert(ship_cargo->amount <= ship_cargo->max);

	amount = MIN(amount, cargo->amount);
	amount = MIN(amount, ship_cargo->max - ship_cargo->amount);

	cargo->amount -= amount;
	ship_cargo->amount += amount;

	return amount;
}

/*
 * Must be called with the appropriate locks (i.e. ship->cargo_lock) held
 */
int move_cargo_from_ship(struct ship * const ship, struct cargo * const cargo, long amount)
{
	struct cargo *ship_cargo;

	assert(cargo->amount >= 0);
	assert(cargo->amount <= cargo->max);
	ship_cargo = st_lookup_string(&ship->cargo_names, cargo->item->name);
	assert(ship_cargo);

	amount = MIN(amount, ship_cargo->amount);
	amount = MIN(amount, cargo->max - cargo->amount);

	ship_cargo->amount -= amount;
	cargo->amount += amount;

	if (!ship_cargo->amount) {
		st_rm_string(&ship->cargo_names, cargo->item->name);
		list_del(&ship_cargo->list);
		cargo_free(ship_cargo);
		free(ship_cargo);
	}

	return amount;
}

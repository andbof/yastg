#include <stdlib.h>
#include <string.h>
#include <common.h>
#include "ship.h"

static void ship_init(struct ship *ship)
{
	memset(ship, 0, sizeof(*ship));
	INIT_LIST_HEAD(&ship->list);
}

void ship_free(struct ship *ship)
{
	if (ship->name)
		free(ship->name);
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

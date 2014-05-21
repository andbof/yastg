#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include "universe.h"
#include "cargo.h"
#include "cli.h"
#include "common.h"
#include "connection.h"
#include "item.h"
#include "log.h"
#include "map.h"
#include "names.h"
#include "port.h"
#include "port_type.h"
#include "planet.h"
#include "planet_type.h"
#include "player.h"
#include "ptrlist.h"
#include "server.h"
#include "ship.h"
#include "star.h"
#include "stringtrie.h"
#include "system.h"

#define player_talk(PLAYER, ...)	\
	conn_send(PLAYER->conn, __VA_ARGS__)

void player_free(struct player *player)
{
	free(player->name);
	cli_tree_destroy(&player->cli);

	struct ship *s, *_s;
	list_for_each_entry_safe(s, _s, &player->ships, list) {
		list_del(&s->list);
		ship_free(s);
		free(s);
	}

	free(player);
}

static char* hundreths(unsigned long l, char *buf, size_t len)
{
	snprintf(buf, len, "%lu.0%lu", l / 100, l % 100);

	return buf;
}

#define MAP_WIDTH 71	/* FIXME: must be uneven for now, or the '|' and the 'X' won't be aligned */
static int cmd_map(void *_player, char *param)
{
	char buf[80 * 50];
	struct player *player = _player;
	assert(player->postype == SHIP);
	struct ship *ship = player->pos;
	assert(ship->postype == SYSTEM);
	struct system *system = ship->pos;

	generate_map(buf, sizeof(buf), system, 50 * TICK_PER_LY, MAP_WIDTH);

	player_talk(player, "%s\n", buf);

	return 0;
}
static char cmd_map_help[] = "Display map";

static void player_showsystem(struct player *player, struct system *system)
{
	struct system *t;
	struct list_head *lh;
	struct star *sol;
	struct planet *planet;
	char buf[10];

	player_talk(player,
		"System %s (coordinates %ldx%ld), habitability %d\n"
		"Habitable zone is from %u to %u Gm\n",
		system->name, system->x, system->y, system->hab, system->hablow, system->habhigh);

	player_talk(player, "Stars:\n");
	ptrlist_for_each_entry(sol, &system->stars, lh)
		player_talk(player,
			"  %s: Class %c %s\n"
			"    Surface temperature: %dK, habitability modifier: %d, luminosity: %s\n",
			sol->name, stellar_cls[sol->cls],
			stellar_lum[sol->lum], sol->temp, sol->hab,
			hundreths(sol->lumval, buf, sizeof(buf)));

	if (!list_empty(&system->planets.list)) {
		player_talk(player, "Planets:\n");
		ptrlist_for_each_entry(planet, &system->planets, lh) {
			player_talk(player,
				"  %s: Class %c (%s)\n"
				"    Diameter: %u km, distance from main star: %u Gm, atmosphere: %s. %s.\n",
				planet->name, planet->type->c, planet->type->name,
				planet->dia*100, planet->dist,
				planet->type->atmo, planet_life_desc[planet->life]);
		}
	} else {
		player_talk(player, "System does not have any planets.\n");
	}

	if (!list_empty(&system->links.list)) {
		player_talk(player, "This system has hyperspace links to\n");
		ptrlist_for_each_entry(t, &system->links, lh)
			player_talk(player, "  %s\n", t->name);
	} else {
		player_talk(player, "This system does not have any hyperspace links.\n");
	}

	struct ptrlist neigh;
	ptrlist_init(&neigh);

	player_talk(player, "Systems within 50 lys are:\n");
	get_neighbouring_systems(&neigh, system, 50 * TICK_PER_LY);
	ptrlist_sort(&neigh, system, cmp_system_distances);
	if (!list_empty(&neigh.list)) {
		ptrlist_for_each_entry(t, &neigh, lh) {
			if (t != system)
				player_talk(player, "  %s at %.1f ly\n", t->name,
						system_distance(system, t) / (double)TICK_PER_LY);
		}
	}

	ptrlist_free(&neigh);

	if (system->owner != NULL) {
		player_talk(player, "This system is owned by civ %s\n", system->owner->name);
	} else {
		player_talk(player, "This system is not part of any civilization\n");
	}
}

static void player_showport(struct player *player, struct port *port)
{
	char *o;
	if (port->planet)
		o = port->planet->name;
	else
		o = port->system->name;

	player_talk(player,
		"Station %s, orbiting %s. %s\n"
		"%s\n",
		port->name, o, port->type->name,
		port->type->desc);
}

static void player_describe_port(struct player *player, struct port *port)
{
	player_talk(player, "%s\n", port->name);
}

static void player_showplanet(struct player *player, struct planet *planet)
{
	struct port *port;
	struct list_head *lh;
	if (planet->gname)
		player_talk(player, "Planet %s (%s) in system %s",
			planet->gname, planet->name, planet->system->name);
	else
		player_talk(player, "Planet %s in system %s",
			planet->name, planet->system->name);
	player_talk(player,
		", class %c (%s).\n"
		"  Diameter: %u km, distance from main star: %u Gm, atmosphere: %s. %s.\n",
		planet->type->c, planet->type->name,
		planet->dia*100, planet->dist, planet->type->atmo,
		planet_life_desc[planet->life]);

	if (!list_empty(&planet->ports.list)) {
		player_talk(player, "Ports:\n");
		ptrlist_for_each_entry(port, &planet->ports, lh) {
			player_talk(player, "  ");
			player_describe_port(player, port);
		}
	} else {
		player_talk(player, "No ports.\n");
	}

	if (!list_empty(&planet->stations.list)) {
		player_talk(player, "Orbital stations:\n");
		ptrlist_for_each_entry(port, &planet->stations, lh) {
			player_talk(player, "  ");
			player_describe_port(player, port);
		}
	} else {
		player_talk(player, "No orbital stations.\n");
	}
}

static int cmd_help(void *ptr, char *param)
{
	struct player *player = ptr;
	cli_print_help(&player->cli, conn_send, player->conn);
	return 0;
}
static char cmd_help_help[] = "Short help on available commands";

static int cmd_quit(void *ptr, char *param)
{
	struct player *player = ptr;
	player_talk(player, "Bye!\n");
	server_disconnect_nicely(player->conn);
	return 0;
}
static char cmd_quit_help[] = "Log off and terminate connection";

static int cmd_look(void *ptr, char *param)
{
	struct player *player = ptr;
	assert(player->postype == SHIP);
	struct ship *ship = player->pos;
	switch (ship->postype) {
	case SYSTEM:
		player_showsystem(player, ship->pos);
		break;
	case PORT:
		player_showport(player, ship->pos);
		break;
	case PLANET:
		player_showplanet(player, ship->pos);
		break;
	default:
		player_talk(player, "internal error: don't know where you are\n");
	}
	return 0;
}
static char cmd_look_help[] = "Look around";

static int cmd_hyper(void *ptr, char *param)
{
	struct player *player = ptr;
	assert(player->postype == SHIP);
	struct ship *ship = player->pos;
	assert(ship->postype == SYSTEM);

	struct system *system;
	pthread_rwlock_rdlock(&univ.systemnames_lock);
	system = st_lookup_string(&univ.systemnames, param);
	pthread_rwlock_unlock(&univ.systemnames_lock);

	if (system == NULL) {
		player_talk(player, "System not found.\n");
		return 1;
	}

	int ok = 0;
	struct system *tmp;
	struct system *pos = ship->pos;
	struct list_head *lh;
	ptrlist_for_each_entry(tmp, &pos->links, lh) {
		if (tmp == system) {
			ok = 1;
			break;
		}
	}

	if (ok) {
		player_talk(player, "Entering %s\n", system->name);
		player_go(player, SYSTEM, system);
		return 0;
	} else {
		player_talk(player, "No hyperspace link found.\n");
		return 1;
	}

}
static char cmd_hyper_help[] = "Travel by hyperspace to system";

static int cmd_jump(void *ptr, char *param)
{
	struct player *player = ptr;
	struct system *system;

	pthread_rwlock_rdlock(&univ.systemnames_lock);
	system = st_lookup_string(&univ.systemnames, param);
	pthread_rwlock_unlock(&univ.systemnames_lock);

	if (system != NULL) {
		player_talk(player, "Jumping to %s\n", system->name);
		player_go(player, SYSTEM, system);
		return 0;
	}

	player_talk(player, "System not found.\n");
	return 1;
}
static char cmd_jump_help[] = "Travel by jumpdrive to system";

static int cmd_dock(void *ptr, char *param)
{
	struct player *player = ptr;
	assert(player->postype == SHIP);
	struct ship *ship = player->pos;

	struct port *port;
	pthread_rwlock_rdlock(&univ.portnames_lock);
	port = st_lookup_string(&univ.portnames, param);
	pthread_rwlock_unlock(&univ.portnames_lock);

	if (port && ((ship->postype == PLANET && port->planet == ship->pos)
		|| (ship->postype == SYSTEM && port->system == ship->pos))) {
			player_talk(player, "Docking at %s\n", port->name);
			player_go(player, PORT, port);
			return 0;
	}

	player_talk(player, "No port or spacedock found by that name.\n");
	return 1;
}
static char cmd_dock_help[] = "Dock at spacedock or port";

static int cmd_orbit(void *ptr, char *param)
{
	struct player *player = ptr;
	assert(player->postype == SHIP);
	struct ship *ship = player->pos;
	assert(ship->postype == SYSTEM);
	struct system *system = ship->pos;

	struct planet *planet;
	pthread_rwlock_rdlock(&univ.planetnames_lock);
	planet = st_lookup_string(&univ.planetnames, param);
	pthread_rwlock_unlock(&univ.planetnames_lock);

	if (planet && planet->system == system) {
		player_talk(player, "Entering orbit around %s\n", planet->name);
		player_go(player, PLANET, planet);
		return 0;
	}

	player_talk(player, "No such planet found in this system.\n");
	return 1;
}
static char cmd_orbit_help[] = "Enter orbit around planet";

static int cmd_leave_port(void *ptr, char *param)
{
	struct player *player = ptr;
	assert(player->postype == SHIP);
	struct ship *ship = player->pos;
	assert(ship->postype == PORT);
	struct port *port = ship->pos;
	if (port->planet)
		player_go(player, PLANET, port->planet);
	else if (port->system)
		player_go(player, SYSTEM, port->system);
	else
		bug("port %s does not have any positional information",
				port->name);

	return 0;
}
static char cmd_leave_port_help[] = "Leave port and take off";

static int cmd_leave_planet(void *ptr, char *param)
{
	struct player *player = ptr;
	assert(player->postype == SHIP);
	struct ship *ship = player->pos;
	assert(ship->postype == PLANET);
	struct planet *planet = ship->pos;
	assert(planet->system);
	player_talk(player, "Leaving orbit around %s\n", planet->name);
	player_go(player, SYSTEM, planet->system);
	return 0;
}
static char cmd_leave_planet_help[] = "Leave planet orbit";

static int cmd_show_ships(void *ptr, char *param)
{
	struct player *player = ptr;
	struct ship *ship;

	player_talk(player, "%-26s %-26s %-32s\n",
			"Name", "Type", "Position");
	list_for_each_entry(ship, &player->ships, list) {
		char pos[64];
		switch (ship->postype) {
		case SYSTEM:
			snprintf(pos, sizeof(pos), "In %s", ((struct system*)ship->pos)->name);
			break;
		case PORT:
			snprintf(pos, sizeof(pos), "Docked at %s", ((struct port*)ship->pos)->name);
			break;
		case PLANET:
			snprintf(pos, sizeof(pos), "Orbiting %s", ((struct planet*)ship->pos)->name);
			break;
		default:
			bug("I don't know where player %s with connection %p is\n", player->name, player->conn);
		}
		pos[sizeof(pos) - 1] = '\0';

		player_talk(player, "%-26.26s %-26.26s %-32.32s\n",
				ship->name, ship->type->name, pos);
	}

	return 0;
}
static char cmd_show_ships_help[] = "Information about your ships";

static int cmd_trade(void *ptr, char *param)
{
	struct player *player = ptr;

	assert(player->postype == SHIP);
	struct ship *ship = player->pos;

	assert(ship->postype == PORT);
	struct port *port = ship->pos;

	pthread_rwlock_rdlock(&port->items_lock);

	struct cargo *c;
	player_talk(player, "%-26s %-12s %-12s %-12s %-12s\n",
			"Item", "In stock", "Max stock", "Daily change", "Price");
	list_for_each_entry(c, &port->items, list) {
		player_talk(player, "%-26.26s %-12ld %-12ld %-12ld %-12ld\n",
				c->item->name, c->amount, c->max, c->daily_change, c->price);
	}

	pthread_rwlock_unlock(&port->items_lock);

	player_talk(player, "\nYou have %ld credits.\n", player->credits);

	return 0;
}
static char cmd_trade_help[] = "Trade with port";

static int parse_buysell_cargo(char * const input, long *amount, char **name)
{
	if (!input)
		return -1;

	char *end;
	end = strchr(input, ' ');
	if (!end)
		return -1;

	*end = '\0';
	*name = end + 1;
	while (isspace(**name))
		name++;

	if (!strcmp(input, "all")) {
		*amount = LONG_MAX;
	} else {
		*amount = strtol(input, &end, 10);
		if (*end != '\0')
			return -1;
		if (*amount <= 0)
			return -1;
	}

	return 0;
}

static const char cmd_buy_syntax[] = "syntax: buy <amount|all> <cargo>\n";
static int cmd_buy(void *ptr, char *param)
{
	struct player *player = ptr;

	assert(player->postype == SHIP);
	struct ship *ship = player->pos;

	assert(ship->postype == PORT);
	struct port *port = ship->pos;

	char *name;
	long amount;
	if (parse_buysell_cargo(param, &amount, &name))
		goto syntax_err;

	pthread_rwlock_wrlock(&port->items_lock);

	struct cargo *c = st_lookup_string(&port->item_names, name);
	if (!c) {
		pthread_rwlock_unlock(&port->items_lock);
		player_talk(player, "%s does not supply %s\n", port->name, name);
		return 0;
	}

	if (c->price) {
		amount = MIN(amount, player->credits / c->price);
		if (!amount) {
			pthread_rwlock_unlock(&port->items_lock);
			player_talk(player, "You cannot afford any %s\n", c->item->name);
			return 0;
		}
	}

	pthread_rwlock_wrlock(&ship->cargo_lock);

	amount = move_cargo_to_ship(ship, c, amount);
	long price = amount * c->price;
	player->credits -= price;

	pthread_rwlock_unlock(&ship->cargo_lock);
	pthread_rwlock_unlock(&port->items_lock);

	if (amount)
		player_talk(player, "Bought %ld %s from %s for %ld credits\n",
				amount, c->item->name, port->name, price);
	else
		player_talk(player, "Cannot buy any %s\n", c->item->name);

	return 0;

syntax_err:
	player_talk(player, "%s", cmd_buy_syntax);
	return 0;
}
static char cmd_buy_help[] = "Buy goods from port";

static const char cmd_sell_syntax[] = "syntax: sell <amount|all> <cargo>\n";
static int cmd_sell(void *ptr, char *param)
{
	struct player *player = ptr;

	assert(player->postype == SHIP);
	struct ship *ship = player->pos;

	assert(ship->postype == PORT);
	struct port *port = ship->pos;

	char *name;
	long amount;
	if (parse_buysell_cargo(param, &amount, &name))
		goto syntax_err;

	pthread_rwlock_wrlock(&port->items_lock);
	pthread_rwlock_wrlock(&ship->cargo_lock);

	if (!st_lookup_string(&ship->cargo_names, name)) {
		player_talk(player, "You don't have any %s\n", name);
		goto unlock;
	}

	struct cargo *c = st_lookup_string(&port->item_names, name);
	if (!c) {
		player_talk(player, "%s does not accept %s\n", port->name, name);
		goto unlock;
	}
	long price;

	amount = move_cargo_from_ship(ship, c, amount);
	price = amount * c->price;
	player->credits += price;

	pthread_rwlock_unlock(&port->items_lock);
	pthread_rwlock_unlock(&ship->cargo_lock);

	if (amount)
		player_talk(player, "Sold %ld %s to %s for %ld credits\n",
				amount, c->item->name, port->name, price);
	else
		player_talk(player, "Cannot sell any %s\n", c->item->name);

	return 0;

unlock:
	pthread_rwlock_unlock(&port->items_lock);
	pthread_rwlock_unlock(&ship->cargo_lock);
	return 0;

syntax_err:
	player_talk(player, "%s", cmd_sell_syntax);
	return 0;
}
static char cmd_sell_help[] = "Sell goods to port";

static int cmd_inventory(void *ptr, char *param)
{
	struct player *player = ptr;

	assert(player->postype == SHIP);
	struct ship *ship = player->pos;

	pthread_rwlock_rdlock(&ship->cargo_lock);

	if (list_empty(&ship->cargo)) {
		pthread_rwlock_unlock(&ship->cargo_lock);
		player_talk(player, "Cargo hold of %s is empty.\n", ship->name);
		return 0;
	}

	player_talk(player, "Cargo manifest of %s\n%-26s %-10s\n",
			ship->name, "Name", "Amount");

	struct cargo *c;
	list_for_each_entry(c, &ship->cargo, list)
		player_talk(player, "%-26.26s %-12ld\n",
				c->item->name, c->amount);

	pthread_rwlock_unlock(&ship->cargo_lock);

	return 0;
}
static char cmd_inventory_help[] = "Display ship cargo manifest";

static struct system* current_player_system(struct player *player)
{
	struct ship *ship;

	switch (player->postype) {
	case SYSTEM:
		return player->pos;
	case PORT:
		return ((struct port*)player->pos)->system;
	case PLANET:
		return ((struct planet*)player->pos)->system;
	case SHIP:
		ship = player->pos;
		switch (ship->postype) {
		case SYSTEM:
			return ship->pos;
		case PORT:
			return ((struct port*)ship->pos)->system;
		case PLANET:
			return ((struct planet*)ship->pos)->system;
		default:
			bug("I don't know where ship %s (player %s, connection %p) is"
					"(postype is %d)\n",
					ship->name, player->name, player->conn,
					player->postype);
		}
	default:
		bug("I don't know where player %s with connection %p is (postype is %d)\n",
				player->name, player->conn, player->postype);
	}
}

#define DEF_PORT_RADIUS "50"
static int cmd_ports(void *_player, char *param)
{
	struct list_head *lh;
	struct ptrlist neigh;
	struct port *port;
	struct player *player = _player;
	struct system *origin = current_player_system(player);
	long dist;

	if (str_to_long((param ? param : DEF_PORT_RADIUS), &dist)) {
		player_talk(player, "error: radius is not numeric\n");
		return 0;
	}
	dist *= TICK_PER_LY;

	ptrlist_init(&neigh);
	get_neighbouring_ports(&neigh, origin, dist);

	if (!ptrlist_len(&neigh)) {
		player_talk(player, "There are no ports within %ld lys\n",
				dist / TICK_PER_LY);
		goto end;
	}

	player_talk(player, "List of ports within %ld lys (%lu ports)\n"
			"%-26s %-26s %-26s %-9s\n",
			dist / TICK_PER_LY, ptrlist_len(&neigh),
			"Name", "Type", "On / orbiting", "Light yrs");

	ptrlist_for_each_entry(port, &neigh, lh)
		player_talk(player, "%-26s %-26s %-26s %9.1f\n",
				port->name, port->type->name,
				(port->planet ? port->planet->name : port->system->name),
				system_distance(origin, port->system) / (double)TICK_PER_LY);

end:
	ptrlist_free(&neigh);
	return 0;
}
static char cmd_ports_help[] = "List ports within radius; if none is specified, default is " DEF_PORT_RADIUS;

void player_go(struct player *player, enum postype postype, void *pos)
{
	assert(player->postype == SHIP);
	struct ship *ship = player->pos;
	switch (ship->postype) {
	case SYSTEM:
		cli_rm_cmd(&player->cli, "go");
		cli_rm_cmd(&player->cli, "map");
		cli_rm_cmd(&player->cli, "jump");
		cli_rm_cmd(&player->cli, "dock");
		cli_rm_cmd(&player->cli, "orbit");
		break;
	case PORT:
		cli_rm_cmd(&player->cli, "buy");
		cli_rm_cmd(&player->cli, "leave");
		cli_rm_cmd(&player->cli, "sell");
		cli_rm_cmd(&player->cli, "trade");
		break;
	case PLANET:
		cli_rm_cmd(&player->cli, "dock");
		cli_rm_cmd(&player->cli, "leave");
		break;
	case NONE:
		break;
	default:
		bug("I don't know where player %s with connection %p is\n", player->name, player->conn);
	}

	if (ship_go(ship, postype, pos))
		player_talk(player, "You're not allowed to go there from here.\n");
	else
		cmd_look(player, NULL);

	switch (ship->postype) {
	case SYSTEM:
		cli_add_cmd(&player->cli, "go", cmd_hyper, player, cmd_hyper_help);
		cli_add_cmd(&player->cli, "map", cmd_map, player, cmd_map_help);
		cli_add_cmd(&player->cli, "jump", cmd_jump, player, cmd_jump_help);
		cli_add_cmd(&player->cli, "dock", cmd_dock, player, cmd_dock_help);
		cli_add_cmd(&player->cli, "orbit", cmd_orbit, player, cmd_orbit_help);
		break;
	case PORT:
		cli_add_cmd(&player->cli, "buy", cmd_buy, player, cmd_buy_help);
		cli_add_cmd(&player->cli, "leave", cmd_leave_port, player, cmd_leave_port_help);
		cli_add_cmd(&player->cli, "sell", cmd_sell, player, cmd_sell_help);
		cli_add_cmd(&player->cli, "trade", cmd_trade, player, cmd_trade_help);
		break;
	case PLANET:
		cli_add_cmd(&player->cli, "dock", cmd_dock, player, cmd_dock_help);
		cli_add_cmd(&player->cli, "leave", cmd_leave_planet, player, cmd_leave_planet_help);
		break;
	case NONE:
		/* Fall through to default as NONE is only valid right after init */
	default:
		bug("I don't know where player %s with connection %p is\n", player->name, player->conn);
	}
}

int player_init(struct player *player)
{
	memset(player, 0, sizeof(*player));
	player->name = create_unique_name(&univ.avail_player_names);
	if (!player->name)
		return -1;

	INIT_LIST_HEAD(&player->list);
	st_init(&player->cli);
	INIT_LIST_HEAD(&player->ships);

	cli_add_cmd(&player->cli, "help", cmd_help, player, cmd_help_help);
	cli_add_cmd(&player->cli, "inventory", cmd_inventory, player, cmd_inventory_help);
	cli_add_cmd(&player->cli, "quit", cmd_quit, player, cmd_quit_help);
	cli_add_cmd(&player->cli, "look", cmd_look, player, cmd_look_help);
	cli_add_cmd(&player->cli, "ships", cmd_show_ships, player, cmd_show_ships_help);
	cli_add_cmd(&player->cli, "ports", cmd_ports, player, cmd_ports_help);

	return 0;
}

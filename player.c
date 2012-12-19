#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "base.h"
#include "cli.h"
#include "common.h"
#include "server.h"
#include "connection.h"
#include "data.h"
#include "stringtree.h"
#include "log.h"
#include "planet.h"
#include "planet_type.h"
#include "player.h"
#include "ptrlist.h"
#include "system.h"
#include "star.h"
#include "universe.h"
#include "names.h"

#define player_talk(PLAYER, ...)	\
	conn_send(PLAYER->conn, __VA_ARGS__)

void player_free(struct player *player)
{
	free(player->name);
	cli_tree_destroy(&player->cli);
	free(player);
}

static char* hundreths(unsigned long l, char *buf, size_t len)
{
	snprintf(buf, len, "%lu.0%lu", l / 100, l % 100);

	return buf;
}

static void player_showsystem(struct player *player, struct system *system)
{
	struct system *t;
	struct list_head *lh;
	struct star *sol;
	struct planet *planet;
	char buf[10];

	player_talk(player,
		"System %s (coordinates %ldx%ld), habitability %d\n"
		"Habitable zone is from %lu to %lu Gm\n",
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

static void player_showbase(struct player *player, struct base *base)
{
	char *o;
	if (base->planet)
		o = base->planet->name;
	else
		o = base->system->name;
	struct base_type *type = &base_types[base->type];

	player_talk(player,
		"Station %s, orbiting %s. %s\n"
		"%s\n",
		base->name, o, type->name,
		type->desc);
}

static void player_describe_base(struct player *player, struct base *base)
{
	player_talk(player, "%s\n", base->name);
}

static void player_showplanet(struct player *player, struct planet *planet)
{
	struct base *base;
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

	if (!list_empty(&planet->bases.list)) {
		player_talk(player, "Bases:\n");
		ptrlist_for_each_entry(base, &planet->bases, lh) {
			player_talk(player, "  ");
			player_describe_base(player, base);
		}
	} else {
		player_talk(player, "No bases.\n");
	}

	if (!list_empty(&planet->stations.list)) {
		player_talk(player, "Orbital stations:\n");
		ptrlist_for_each_entry(base, &planet->stations, lh) {
			player_talk(player, "  ");
			player_describe_base(player, base);
		}
	} else {
		player_talk(player, "No orbital stations.\n");
	}
}

static int cmd_help(void *ptr, char *param)
{
	struct player *player = ptr;
	/* FIXME: This should use conn_send instead of writing directly to the fd */
	FILE *f = fdopen(dup(player->conn->peerfd), "w");
	cli_print_help(f, &player->cli);
	fclose(f);
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
	switch (player->postype) {
	case SYSTEM:
		player_showsystem(player, player->pos);
		break;
	case BASE:
		player_showbase(player, player->pos);
		break;
	case PLANET:
		player_showplanet(player, player->pos);
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
	assert(player->postype == SYSTEM);

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
	struct system *pos = player->pos;
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

	struct base *base;
	pthread_rwlock_rdlock(&univ.basenames_lock);
	base = st_lookup_string(&univ.basenames, param);
	pthread_rwlock_unlock(&univ.basenames_lock);

	if (base && ((player->postype == PLANET && base->planet == player->pos)
		|| (player->postype == SYSTEM && base->system == player->pos))) {
			player_talk(player, "Docking at %s\n", base->name);
			player_go(player, BASE, base);
			return 0;
	}

	player_talk(player, "No base or spacedock found by that name.\n");
	return 1;
}
static char cmd_dock_help[] = "Dock at spacedock or base";

static int cmd_orbit(void *ptr, char *param)
{
	struct player *player = ptr;
	assert(player->postype == SYSTEM);
	struct system *system = player->pos;

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

static int cmd_leave_base(void *ptr, char *param)
{
	struct player *player = ptr;
	assert(player->postype == BASE);
	struct base *base = player->pos;
	if (base->planet)
		player_go(player, PLANET, base->planet);
	else if (base->system)
		player_go(player, SYSTEM, base->system);
	else
		bug("%s", "base %s does not have any positional information");

	return 0;
}
static char cmd_leave_base_help[] = "Leave base and take off";

static int cmd_leave_planet(void *ptr, char *param)
{
	struct player *player = ptr;
	assert(player->postype == PLANET);
	struct planet *planet = player->pos;
	assert(planet->system);
	player_talk(player, "Leaving orbit around %s\n", planet->name);
	player_go(player, SYSTEM, planet->system);
	return 0;
}
static char cmd_leave_planet_help[] = "Leave planet orbit";

void player_go(struct player *player, enum postype postype, void *pos)
{
	switch (player->postype) {
	case SYSTEM:
		cli_rm_cmd(&player->cli, "jump");
		cli_rm_cmd(&player->cli, "dock");
		cli_rm_cmd(&player->cli, "orbit");
		break;
	case BASE:
		cli_rm_cmd(&player->cli, "leave");
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

	player->postype = postype;
	player->pos = pos;
	cmd_look(player, NULL);

	switch (postype) {
	case SYSTEM:
		cli_add_cmd(&player->cli, "jump", cmd_jump, player, cmd_jump_help);
		cli_add_cmd(&player->cli, "dock", cmd_dock, player, cmd_dock_help);
		cli_add_cmd(&player->cli, "orbit", cmd_orbit, player, cmd_orbit_help);
		break;
	case BASE:
		cli_add_cmd(&player->cli, "leave", cmd_leave_base, player, cmd_leave_base_help);
		break;
	case PLANET:
		cli_add_cmd(&player->cli, "dock", cmd_dock, player, cmd_dock_help);
		cli_add_cmd(&player->cli, "leave", cmd_leave_planet, player, cmd_leave_planet_help);
		break;
	case NONE:
		/* Fall through to default as NONE can only valid right after init */
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
	INIT_LIST_HEAD(&player->cli);
	player->postype = NONE;

	cli_add_cmd(&player->cli, "help", cmd_help, player, cmd_help_help);
	cli_add_cmd(&player->cli, "go", cmd_hyper, player, cmd_hyper_help);
	cli_add_cmd(&player->cli, "quit", cmd_quit, player, cmd_quit_help);
	cli_add_cmd(&player->cli, "look", cmd_look, player, cmd_look_help);

	return 0;
}

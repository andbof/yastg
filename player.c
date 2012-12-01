#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "base.h"
#include "cli.h"
#include "common.h"
#include "connection.h"
#include "data.h"
#include "htable.h"
#include "log.h"
#include "planet.h"
#include "player.h"
#include "ptrlist.h"
#include "sector.h"
#include "star.h"
#include "universe.h"
#include "names.h"

#define player_talk(PLAYER, ...)	\
	conn_send(PLAYER->conn, __VA_ARGS__)

void player_free(void *ptr)
{
	struct player *player = (struct player*)ptr;
	free(player->name);
	cli_tree_destroy(&player->cli);
	free(player);
}

static char* hundreths(unsigned long l)
{
	int mod = l%100;
	char* result;
	MALLOC_DIE(result, (size_t)10);
	if (mod < 10) {
		sprintf(result, "%lu.0%lu", l/100, l%100);
	} else {
		sprintf(result, "%lu.0%lu", l/100, l%100);
	}
	return result;
}

static void player_showsector(struct player *player, struct sector *sector)
{
	struct sector *t;
	struct list_head *lh;
	struct star *sol;
	struct planet *planet;
	player_talk(player,
		"Sector %s (coordinates %ldx%ld), habitability %d\n"
		"Habitable zone is from %lu to %lu Gm\n",
		sector->name, sector->x, sector->y, sector->hab, sector->hablow, sector->habhigh);

	player_talk(player, "Stars:\n");
	ptrlist_for_each_entry(sol, &sector->stars, lh) {
		char *string = hundreths(sol->lumval);
		player_talk(player,
			"  %s: Class %c %s\n"
			"    Surface temperature: %dK, habitability modifier: %d, luminosity: %s\n",
			sol->name, stellar_cls[sol->cls], stellar_lum[sol->lum], sol->temp, sol->hab, string);
		free(string);
	}

	if (!list_empty(&sector->planets.list)) {
		player_talk(player, "Planets:\n");
		ptrlist_for_each_entry(planet, &sector->planets, lh) {
			struct planet_type *type = &planet_types[planet->type];
			player_talk(player,
				"  %s: Class %c (%s)\n"
				"    Diameter: %u km, distance from main star: %u Gm, atmosphere: %s. %s.\n",
				planet->name, type->c, type->name,
				planet->dia*100, planet->dist,
				type->atmo, planet_life_desc[planet->life]);
		}
	} else {
		player_talk(player, "Sector does not have any planets.\n");
	}

	if (!list_empty(&sector->links.list)) {
		player_talk(player, "This sector has hyperspace links to\n");
		ptrlist_for_each_entry(t, &sector->links, lh)
			player_talk(player, "  %s\n", t->name);
	} else {
		player_talk(player, "This sector does not have any hyperspace links.\n");
	}

	player_talk(player, "Sectors within 50 lys are:\n");

	/* FIXME: getneighbours() is awful */
	struct ptrlist *neigh = getneighbours(sector, 50);
	if (!list_empty(&neigh->list)) {
		ptrlist_for_each_entry(t, neigh, lh) {
			if (t != sector)
				player_talk(player, "  %s at %lu ly\n", t->name, sector_distance(sector, t));
		}
	} else {
		player_talk(player, "No sectors within 50 lys.\n");
	}
	ptrlist_free(neigh);
	free(neigh);

	if (sector->owner != NULL) {
		player_talk(player, "This sector is owned by civ %s\n", sector->owner->name);
	} else {
		player_talk(player, "This sector is not part of any civilization\n");
	}
}

static void player_showbase(struct player *player, struct base *base)
{
	char *o;
	if (base->planet)
		o = base->planet->name;
	else
		o = base->sector->name;
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
	struct planet_type *ptype = &planet_types[planet->type];
	struct base *base;
	struct list_head *lh;
	if (planet->gname)
		player_talk(player, "Planet %s (%s) in sector %s",
			planet->gname, planet->name, planet->sector->name);
	else
		player_talk(player, "Planet %s in sector %s",
			planet->name, planet->sector->name);
	player_talk(player,
		", class %c (%s).\n"
		"  Diameter: %u km, distance from main star: %u Gm, atmosphere: %s. %s.\n",
		ptype->c, ptype->name,
		planet->dia*100, planet->dist, ptype->atmo,
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
	conn_cleanexit(player->conn);
	return 0;
}
static char cmd_quit_help[] = "Log off and terminate connection";

static int cmd_look(void *ptr, char *param)
{
	struct player *player = ptr;
	switch (player->postype) {
	case SECTOR:
		player_showsector(player, player->pos);
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
	assert(player->postype == SECTOR);

	pthread_rwlock_rdlock(&univ->sectornames->lock);
	struct sector *sector = htable_get(univ->sectornames, param);
	pthread_rwlock_unlock(&univ->sectornames->lock);

	if (sector == NULL) {
		player_talk(player, "Sector not found.\n");
		return 1;
	}

	int ok = 0;
	struct sector *tmp;
	struct sector *pos = player->pos;
	struct list_head *lh;
	ptrlist_for_each_entry(tmp, &pos->links, lh) {
		if (tmp == sector) {
			ok = 1;
			break;
		}
	}

	if (ok) {
		player_talk(player, "Entering %s\n", sector->name);
		player_go(player, SECTOR, sector);
		return 0;
	} else {
		player_talk(player, "No hyperspace link found.\n");
		return 1;
	}

}
static char cmd_hyper_help[] = "Travel by hyperspace to sector";

static int cmd_jump(void *ptr, char *param)
{
	struct player *player = ptr;
	struct sector *sector = htable_get(univ->sectornames, param);
	if (sector != NULL) {
		player_talk(player, "Jumping to %s\n", sector->name);
		player_go(player, SECTOR, sector);
		return 0;
	}

	player_talk(player, "Sector not found.\n");
	return 1;
}
static char cmd_jump_help[] = "Travel by jumpdrive to sector";

static int cmd_dock(void *ptr, char *param)
{
	struct player *player = ptr;

	pthread_rwlock_rdlock(&univ->sectornames->lock);
	struct base *base = htable_get(univ->basenames, param);
	pthread_rwlock_unlock(&univ->sectornames->lock);

	if (base && ((player->postype == PLANET && base->planet == player->pos)
		|| (player->postype == SECTOR && base->sector == player->pos))) {
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
	assert(player->postype == SECTOR);
	struct sector *sector = player->pos;
	struct list_head *lh;

	pthread_rwlock_rdlock(&univ->planetnames->lock);
	struct planet *planet = htable_get(univ->planetnames, param);
	pthread_rwlock_unlock(&univ->planetnames->lock);

	if (planet && planet->sector == sector) {
		player_talk(player, "Entering orbit around %s\n", planet->name);
		player_go(player, PLANET, planet);
		return 0;
	}

	player_talk(player, "No such planet found in this sector.\n");
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
	else if (base->sector)
		player_go(player, SECTOR, base->sector);
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
	assert(planet->sector);
	player_talk(player, "Leaving orbit around %s\n", planet->name);
	player_go(player, SECTOR, planet->sector);
	return 0;
}
static char cmd_leave_planet_help[] = "Leave planet orbit";

void player_go(struct player *player, enum postype postype, void *pos)
{
	switch (player->postype) {
	case SECTOR:
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
	case SECTOR:
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

void player_init(struct player *player)
{
	memset(player, 0, sizeof(*player));
	player->name = names_generate(&univ->avail_player_names);
	INIT_LIST_HEAD(&player->list);
	INIT_LIST_HEAD(&player->cli);
	player->postype = NONE;

	cli_add_cmd(&player->cli, "help", cmd_help, player, cmd_help_help);
	cli_add_cmd(&player->cli, "go", cmd_hyper, player, cmd_hyper_help);
	cli_add_cmd(&player->cli, "quit", cmd_quit, player, cmd_quit_help);
	cli_add_cmd(&player->cli, "look", cmd_look, player, cmd_look_help);
}

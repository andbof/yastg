#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include "common.h"
#include "player.h"
#include "ptrlist.h"
#include "connection.h"
#include "sector.h"
#include "planet.h"
#include "base.h"
#include "star.h"
#include "log.h"
#include "universe.h"
#include "cli.h"

static void player_go(struct player *player, enum postype postype, void *pos);

#define player_talk(PLAYER, ...)	\
	conn_send(PLAYER->conn, __VA_ARGS__);

void player_free(void *ptr)
{
	struct player *player = (struct player*)ptr;
	free(player->name);
	cli_tree_destroy(player->cli);
	free(player);
}

static char* hundreths(unsigned long l)
{
	int mod = l%100;
	char* result;
	MALLOC_DIE(result, 10);
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
	player_talk(player,
		"You are in sector %s (coordinates %ldx%ld), habitability %d\n"
		"Snow line at %lu Gm\n"
		"Habitable zone is from %lu to %lu Gm\n",
		sector->name, sector->x, sector->y, sector->hab, sector->snowline, sector->hablow, sector->habhigh);
	ptrlist_for_each_entry(sol, sector->stars, lh) {
		char *string = hundreths(sol->lumval);
		player_talk(player,
			"%s: Class %c %s\n"
			"  Surface temperature: %dK, habitability modifier: %d, luminosity: %s\n",
			sol->name, stellar_cls[sol->cls], stellar_lum[sol->lum], sol->temp, sol->hab, string);
		free(string);
	}

	player_talk(player, "This sector has hyperspace links to\n");
	ptrlist_for_each_entry(t, sector->links, lh)
		player_talk(player, "  %s\n", t->name);

	player_talk(player, "Sectors within 50 lys are:\n");
	/* FIXME: getneighbours() is awful */
	struct ptrlist *neigh = getneighbours(sector, 50);
	ptrlist_for_each_entry(t, neigh, lh) {
		if (t != sector)
			player_talk(player, "  %s at %lu ly\n", t->name, sector_distance(sector, t));
	}
	ptrlist_free(neigh);

	if (sector->owner != NULL) {
		player_talk(player, "This sector is owned by civ %s\n", sector->owner->name);
	} else {
		player_talk(player, "This sector is not part of any civilization\n");
	}
}

static void player_showbase(struct player *player, struct base *base)
{
}

static void player_showplanet(struct player *player, struct planet *planet)
{
}

static int cmd_help(void *ptr, char *param)
{
	struct player *player = ptr;
	/* FIXME: This should use conn_send instead of writing directly to the fd */
	FILE *f = fdopen(dup(player->conn->peerfd), "w");
	cli_print_help(f, player->cli);
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
	if (param[0] != '\0') {
		struct sector *sector = getsectorbyname(univ, param);
		if (sector != NULL) {
			player_talk(player, "Entering %s\n", sector->name);
			player_go(player, SECTOR, sector);
		} else {
			player_talk(player, "Sector not found.\n");
		}
		return 0;
	} else {
		player_talk(player, ERR_SYNTAX);
		return 1;
	}
}
static char cmd_hyper_help[] = "Travel by hyperspace to sector";

static int cmd_jump(void *ptr, char *param)
{
	struct player *player = ptr;
	return 0;
}
static char cmd_jump_help[] = "Travel by jumpdrive to sector";

static int cmd_dock(void *ptr, char *param)
{
	struct player *player = ptr;
	return 0;
}
static char cmd_dock_help[] = "Dock at spacedock or base";

static int cmd_orbit(void *ptr, char *param)
{
	struct player *player = ptr;
	return 0;
}
static char cmd_orbit_help[] = "Enter orbit around planet";

static int cmd_leave_base(void *ptr, char *param)
{
	struct player *player = ptr;
	return 0;
}
static char cmd_leave_base_help[] = "Leave base and take off";

static int cmd_leave_planet(void *ptr, char *param)
{
	struct player *player = ptr;
	return 0;
}
static char cmd_leave_planet_help[] = "Leave planet orbit";

static void player_go(struct player *player, enum postype postype, void *pos)
{
	switch (player->postype) {
	case SECTOR:
		cli_rm_cmd(player->cli, "jump");
		cli_rm_cmd(player->cli, "dock");
		cli_rm_cmd(player->cli, "orbit");
		break;
	case BASE:
		cli_rm_cmd(player->cli, "leave");
		break;
	case PLANET:
		cli_rm_cmd(player->cli, "leave");
		break;
	case NONE:
		break;
	default:
		bug("I don't know where player %zx is\n", player->conn->id);
	}

	player->postype = postype;
	player->pos = pos;
	cmd_look(player, NULL);

	switch (postype) {
	case SECTOR:
		cli_add_cmd(player->cli, "jump", cmd_jump, player, cmd_jump_help);
		cli_add_cmd(player->cli, "dock", cmd_dock, player, cmd_dock_help);
		cli_add_cmd(player->cli, "orbit", cmd_orbit, player, cmd_orbit_help);
		break;
	case BASE:
		cli_add_cmd(player->cli, "leave", cmd_leave_base, player, cmd_leave_base_help);
		break;
	case PLANET:
		cli_add_cmd(player->cli, "leave", cmd_leave_planet, player, cmd_leave_planet_help);
		break;
	case NONE:
		/* Fall through to default as NONE can only valid right after init */
	default:
		bug("I don't know where player %zx is\n", player->conn->id);
	}
}

void player_init(struct player *player)
{
	/* FIXME: We should probably do a memset(0) here, but that will destroy
	 * whatever info is put in here by conn_main(). */
	INIT_LIST_HEAD(&player->list);
	player->cli = cli_tree_create();
	player->postype = NONE;

	cli_add_cmd(player->cli, "help", cmd_help, player, cmd_help_help);
	cli_add_cmd(player->cli, "go", cmd_hyper, player, cmd_hyper_help);
	cli_add_cmd(player->cli, "quit", cmd_quit, player, cmd_quit_help);
	cli_add_cmd(player->cli, "look", cmd_look, player, cmd_look_help);

	player_go(player, SECTOR, ptrlist_entry(univ->sectors, 0));
}

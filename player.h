#ifndef _HAS_PLAYER_H
#define _HAS_PLAYER_H

#include "cli.h"
#include "list.h"

enum postype {
	NONE,
	SECTOR,
	BASE,
	PLANET
};

struct player {
	char *name;
	enum postype postype;
	void *pos;
	struct list_head list;
	struct cli_tree *cli;
	struct conndata *conn;
};

void player_init(struct player *player);
void player_free();
void player_talk(struct player *player, char *format, ...);

static struct player *alfred;

#endif

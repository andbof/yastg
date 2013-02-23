#ifndef _HAS_PLAYER_H
#define _HAS_PLAYER_H

#include "list.h"
#include "ship.h"

struct player {
	char *name;
	long credits;
	enum postype postype;
	void *pos;
	struct list_head ships;
	struct list_head list;
	struct list_head cli;
	struct connection *conn;
};

int player_init(struct player *player);
void player_free(struct player *player);
void player_talk(struct player *player, char *format, ...);
void player_go(struct player *player, enum postype postype, void *pos);
void player_change_ship(struct player *player, struct ship *ship);

#endif

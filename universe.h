#ifndef _HAS_UNIVERSE_H
#define _HAS_UNIVERSE_H

#include "list.h"
#include "rbtree.h"
#include "civ.h"
#include "sector.h"
#include "names.h"

struct universe {
	size_t id;			/* ID of the universe (or the game?) */
	char* name;			/* The name of the universe (or the game?) */
	time_t created;			/* When the universe was created */
	size_t numsector;
	struct ptrlist sectors;
	struct rb_root x_rbtree;
	struct list_head sectornames;
	pthread_rwlock_t sectornames_lock;
	struct list_head planetnames;
	pthread_rwlock_t planetnames_lock;
	struct list_head basenames;
	pthread_rwlock_t basenames_lock;
	struct list_head list;
	struct name_list avail_base_names;
	struct name_list avail_player_names;
};

extern struct universe univ;

void universe_free(struct universe *u);
struct universe* universe_create();
void universe_init(struct universe *u);
int universe_genesis(struct universe *univ, struct civ *civs);

unsigned long get_neighbouring_systems(struct ptrlist * const neighbours,
		const struct sector * const origin, const long max_distance);

int sector_move(struct sector * const s, const long x, const long y);
int makeneighbours(struct sector *s1, struct sector *s2, unsigned long min, unsigned long max);
void linksectors(struct sector *s1, struct sector *s2);

#endif

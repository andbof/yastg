#ifndef _HAS_UNIVERSE_H
#define _HAS_UNIVERSE_H

#include "list.h"
#include "civ.h"
#include "sarray.h"
#include "sector.h"
#include "names.h"

struct universe {
	size_t id;			/* ID of the universe (or the game?) */
	char* name;			/* The name of the universe (or the game?) */
	struct tm *created;		/* When the universe was created */
	size_t numsector;
	struct ptrlist *sectors;
	struct htable *sectornames;
	struct htable *planetnames;
	struct htable *basenames;
	struct sarray *srad;		/* Sector IDs sorted by radial position from (0,0) */
	struct sarray *sphi;		/* Sector IDs sorted by angle from (0,0) */
	struct list_head list;
	struct name_list avail_base_names;
	struct name_list avail_player_names;
};

struct universe *univ;

void universe_free(struct universe *u);
struct universe* universe_create();
void universe_init(struct civ *civs);
size_t countneighbours(struct sector *s, unsigned long dist);
struct ptrlist* getneighbours(struct sector *s, unsigned long dist);
int makeneighbours(struct sector *s1, struct sector *s2, unsigned long min, unsigned long max);
void linksectors(struct sector *s1, struct sector *s2);

#endif

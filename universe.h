#ifndef HAS_UNIVERSE_H
#define HAS_UNIVERSE_H

#include "list.h"
#include "civ.h"

struct sarray;
struct sector;

struct universe {
	size_t id;			/* ID of the universe (or the game?) */
	char* name;			/* The name of the universe (or the game?) */
	struct tm *created;		/* When the universe was created */
	size_t numsector;		/* Number of sectors in universe */
	struct ptrlist *sectors;	/* Sectors in universe */
	struct htable *sectornames;	/* Sector names, indexed in a string table */
	struct sarray *srad;		/* Sector IDs sorted by radial position from (0,0) */
	struct sarray *sphi;		/* Sector IDs sorted by angle from (0,0) */
	struct list_head list;
};

struct universe *univ;

void universe_free(struct universe *u);
struct universe* universe_create();
void universe_init(struct civ *civs);
size_t countneighbours(struct sector *s, unsigned long dist);
struct ptrlist* getneighbours(struct sector *s, unsigned long dist);
int makeneighbours(struct sector *s1, struct sector *s2, unsigned long min, unsigned long max);
void linksectors(struct sector *s1, struct sector *s2);

struct sector* getsectorbyname(struct universe *u, char *name);

#endif

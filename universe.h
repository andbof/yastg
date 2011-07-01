#ifndef HAS_UNIVERSE_H
#define HAS_UNIVERSE_H

#include "sector.h"

struct universe {
  size_t id;			// ID of the universe (or the game?)
  char* name;			// The name of the universe (or the game?)
  struct tm *created;		// When the universe was created
  size_t numsector;		// Number of sectors in universe
  struct sarray *sectors;	// Sectors in universe
  struct sarray *srad;		// Sector IDs sorted by radial position from (0,0)
  struct sarray *sphi;		// Sector IDs sorted by angle from (0,0)
};

struct universe *univ;

void universe_free(struct universe *u);
struct universe* createuniverse(struct sarray *civs);
size_t countneighbours(struct universe *u, struct sector *s, unsigned long dist);
struct sarray* getneighbours(struct universe *u, struct sector *s, unsigned long dist);
int makeneighbours(struct universe *u, size_t id1, size_t id2, unsigned long min, unsigned long max);
void linksectors(struct universe *u, size_t id1, size_t id2);

#endif

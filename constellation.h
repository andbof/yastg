#ifndef _HAS_CONSTELLATION_H
#define _HAS_CONSTELLATION_H

#define CONSTELLATION_NEIGHBOUR_CHANCE (5 * TICK_PER_LY)
#define CONSTELLATION_MIN_DISTANCE (150 * TICK_PER_LY)
#define CONSTELLATION_RANDOM_DISTANCE (500 * TICK_PER_LY)
#define CONSTELLATION_PHI_RANDOM 1.0
#define CONSTELLATION_MAXNUM 128

int spawn_constellations(struct universe *u);

#endif

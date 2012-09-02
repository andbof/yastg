#ifndef _HAS_STAR_H
#define _HAS_STAR_H

#include "sector.h"
#include "parseconfig.h"

#define STELLAR_LUM_N 9
#define STELLAR_CLS_N 7
extern const char *stellar_lum[STELLAR_LUM_N];
extern const char stellar_cls[STELLAR_CLS_N];

struct star {
	char* name;
	int cls, lum, hab;
	unsigned int lumval;
	unsigned int hablow, habhigh;
	unsigned int temp;
};

struct star* loadstar(struct configtree *ctree);

unsigned long star_gethablow(unsigned int lumval);
unsigned long star_gethabhigh(unsigned int lumval);

void star_populate_sector(struct sector *sector);
void star_free(struct star *s);

#endif

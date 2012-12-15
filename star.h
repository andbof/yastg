#ifndef _HAS_STAR_H
#define _HAS_STAR_H

#include "system.h"
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

int star_load(struct star *sol, const struct list_head * const config_root);

unsigned long star_gethablow(unsigned int lumval);
unsigned long star_gethabhigh(unsigned int lumval);

int star_populate_system(struct system *system);
void star_free(struct star *s);

#endif

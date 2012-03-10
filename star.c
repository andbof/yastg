#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "defines.h"
#include "log.h"
#include "star.h"
#include "ptrarray.h"
#include "parseconfig.h"
#include "mtrandom.h"

// Stellar luminosity classes
const char *stellar_lum[STELLAR_LUM_N] = {
	"0",  "Ia", "Ib", "II", "III", "IV", "V",  "VI", "VII"
};
// Odds for each luminosity class
const int stellar_lumodds[STELLAR_LUM_N] = {
	1,    2,    4,    8,    25,    32,   75,   98,   100
};
// Habitability modifiers for each luminosity class
const int stellar_lumhab[STELLAR_LUM_N] = {
	-100, -90,  -90,  -70,  -30,   0,    +100, +80,  +80
};

// Spectral classes
const char stellar_cls[STELLAR_CLS_N] = {
	'O',   'B',    'A',  'F', 'G',  'K',  'M'
};
// Odds for each spectral class
const int stellar_clsodds[STELLAR_CLS_N] = {
	3,     19,     83,   283, 1043, 2253, 10000
};
// Luminosity ceiling per luminosity class, defined in hundreth of solar units
const unsigned long stellar_clslum[STELLAR_CLS_N+1] = {
	9999900,3000000,2500,500, 150,  60,   8, 	1
};
// Temperature ceiling per luminosity class, defined in Kelvin
const unsigned int stellar_clstmp[STELLAR_CLS_N+1] = {
	99999, 33000,  10000,7500,6000, 5200, 3700,	2500
};
// Odds for multiple systems for each class
// FIXME: This is not used
const int stellar_clsmul[STELLAR_CLS_N] = {
	95,   90,    85,  70,  40,   30,   20
};
// Habitability modifiers for each class
const int stellar_clshab[STELLAR_CLS_N] = {
	-200, -150, -100, -50, +10,  +30, +50
};

/*
 * Parses a configuration tree and returns a struct star*
 */
struct star* loadstar(struct configtree *ctree) {
	struct star *sol;
	int i;
	int habset = 0;
	int tempset = 0;
	int lumset = 0;
	int lumvalset = 0;
	int clsset = 0;
	MALLOC_DIE(sol, sizeof(*sol));
	sol->temp = 0;
	sol->hab = 0;
	sol->name = NULL;
	while (ctree) {
		if (strcmp(ctree->key, "NAME") == 0) {
			sol->name = strdup(ctree->data);
		} else if (strcmp(ctree->key, "CLASS") == 0) {
			if (strlen(ctree->data) != 1)
				die("invalid stellar class in configuration file: %s\n", ctree->data);
			for (i = 0; i < STELLAR_CLS_N; i++) {
				if (ctree->data[0] == stellar_cls[i]) {
					sol->cls = i;
					clsset = 1;
					break;
				}
			}
			if (!clsset)
				die("invalid stellar class in configuration file: %s\n", ctree->data);
		} else if (strcmp(ctree->key, "LUM") == 0) {
			for (i = 0; i < STELLAR_LUM_N; i++) {
				if (strcmp(ctree->data, stellar_lum[i]) == 0) {
					sol->lum = i;
					lumset = 1;
					break;
				}
			}
			if (!lumset)
				die("invalid stellar luminosity in configuration file: %s\n", ctree->data);
		} else if (strcmp(ctree->key, "LUMVAL") == 0) {
			sscanf(ctree->data, "%lu", &(sol->lumval));
			lumvalset = 1;
		} else if (strcmp(ctree->key, "TEMP") == 0) {
			sscanf(ctree->data, "%u", &(sol->temp));
			tempset = 1;
		} else if (strcmp(ctree->key, "HAB") == 0) {
			sscanf(ctree->data, "%u", &(sol->hab));
			habset = 1;
		} else {
			printf("warning: invalid entry in configuration file: %s %s\n", ctree->key, ctree->data);
		}
		ctree = ctree->next;
	}
	if (!habset)
		sol->hab = 0;
	if (!sol->name)
		die("%s", "required attribute missing for predefined star: stellar name");
	if (!clsset)
		die("required attribute missing for predefined star %s: stellar class", sol->name);
	if (!lumset)
		die("required attribute missing for predefined star %s: stellar luminosity", sol->name);
	// FIXME:
	if (!lumvalset)
		sol->lumval = stellar_clslum[sol->cls + 1] + mtrandom_sizet(stellar_clslum[sol->cls]) - stellar_clslum[sol->cls + 1];
	sol->hab = stellar_clshab[sol->cls] + stellar_lumhab[sol->lum];
	if (!tempset)
		sol->temp = stellar_clstmp[sol->cls + 1] + mtrandom_sizet(stellar_clstmp[sol->cls])-stellar_clstmp[sol->cls + 1];
	// Rough guess looking at
	// https://secure.wikimedia.org/wikipedia/en/wiki/Habitable_zone
	sol->hablow = star_gethablow(sol);
	sol->habhigh = star_gethabhigh(sol);
	sol->snowline = star_getsnowline(sol);
	return sol;
}

/*
 * Returns a random star luminosity
 */
int star_genlum(void) {
	unsigned int chance = mtrandom_uint(stellar_lumodds[STELLAR_LUM_N - 1]);
	int i;
	for (i = 0; i < STELLAR_LUM_N; i++) {
		if (chance < stellar_lumodds[i])
			return i;
	}
	bug("%s", "illegal execution point");
}


/*
 * Returns a random star classification.
 */
int star_gencls(void) {
	unsigned int chance = mtrandom_uint(stellar_clsodds[STELLAR_CLS_N - 1]);
	int i;
	for (i = 0; i < STELLAR_CLS_N; i++) {
		if (chance < stellar_clsodds[i])
			return i;
	}
	bug("%s", "illegal execution point");
}

/*
 * Returns the number of stars to be created
 */
int star_gennum() {
	int num = 1;
	while (mtrandom_sizet(SIZE_MAX) - SIZE_MAX/STELLAR_MUL_ODDS < 0) num++;
	if (num > STELLAR_MUL_MAX)
		num = STELLAR_MUL_MAX;
	return num;
}

unsigned long star_gethablow(struct star *s) {
	// Rough guess looking at
	// https://secure.wikimedia.org/wikipedia/en/wiki/Habitable_zone
	return sqrt((double)s->lumval/100.0)*HAB_ZONE_START*GM_PER_AU;
}

unsigned long star_gethabhigh(struct star *s) {
	return sqrt((double)s->lumval/100.0)*HAB_ZONE_END*GM_PER_AU;
}

unsigned long star_getsnowline(struct star *s) {
	return sqrt((double)s->lumval/100.0)*SNOW_LINE;
}

struct star* createstar() {
	struct star *s;
	MALLOC_DIE(s, sizeof(struct star));
	s->name = NULL;
	s->cls = star_gencls();
	s->lum = star_genlum();
	s->lumval = stellar_clslum[s->cls+1] + mtrandom_sizet(stellar_clslum[s->cls]) - stellar_clslum[s->cls + 1];
	s->hab = stellar_clshab[s->cls] + stellar_lumhab[s->lum];
	s->temp = stellar_clstmp[s->cls+1] + mtrandom_sizet(stellar_clstmp[s->cls]) - stellar_clstmp[s->cls + 1];
	s->hablow = star_gethablow(s);
	s->habhigh = star_gethabhigh(s);
	s->snowline = star_getsnowline(s);
	return s;
}

struct ptrarray* createstars() {
	struct star *s;
	int i;
	struct ptrarray *a = ptrarray_init(0);
	int num = star_gennum();
	for (i = 0; i < num; i++) {
		s = createstar();
		ptrarray_push(a, s);
	}
	return a;
}

void star_free(void *ptr) {
	struct star *s = ptr;
	free(s->name);
	free(s);
}

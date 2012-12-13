#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "common.h"
#include "log.h"
#include "star.h"
#include "ptrlist.h"
#include "parseconfig.h"
#include "mtrandom.h"

/* Stellar luminosity classes */
const char *stellar_lum[STELLAR_LUM_N] = {
	"0",  "Ia", "Ib", "II", "III", "IV", "V",  "VI", "VII"
};
/* Odds for each luminosity class */
const int stellar_lumodds[STELLAR_LUM_N] = {
	1,    2,    4,    8,    25,    32,   75,   98,   100
};
/* Habitability modifiers for each luminosity class */
const int stellar_lumhab[STELLAR_LUM_N] = {
	-100, -90,  -90,  -70,  -30,   0,    +100, +80,  +80
};

/* Spectral classes */
const char stellar_cls[STELLAR_CLS_N] = {
	'O',   'B',    'A',  'F', 'G',  'K',  'M'
};

/* Odds for each spectral class */
const int stellar_clsodds[STELLAR_CLS_N] = {
	3,     19,     83,   283, 1043, 2253, 10000
};

/* Luminosity ceiling per luminosity class, defined in hundreth of solar units */
const unsigned int stellar_clslum[STELLAR_CLS_N+1] = {
	9999900,3000000,2500,500, 150,  60,   8, 	1
};

/* Temperature ceiling per luminosity class, defined in Kelvin */
const unsigned int stellar_clstmp[STELLAR_CLS_N+1] = {
	99999, 33000,  10000,7500,6000, 5200, 3700,	2500
};

/* Odds for multiple systems for each class */
#define STELLAR_CLSMUL_MAX 100
const int stellar_clsmul[STELLAR_CLS_N] = {
	95,   90,    85,  70,  40,   30,   20
};

/* Habitability modifiers for each class */
const int stellar_clshab[STELLAR_CLS_N] = {
	-200, -150, -100, -50, +10,  +30, +50
};

static int star_genlum(void)
{
	unsigned int chance = mtrandom_uint(stellar_lumodds[STELLAR_LUM_N - 1]);
	int i;
	for (i = 0; i < STELLAR_LUM_N; i++) {
		if (chance < stellar_lumodds[i])
			return i;
	}
	bug("%s", "illegal execution point");
}


static int star_gencls(void)
{
	unsigned int chance = mtrandom_uint(stellar_clsodds[STELLAR_CLS_N - 1]);
	int i;
	for (i = 0; i < STELLAR_CLS_N; i++) {
		if (chance < stellar_clsodds[i])
			return i;
	}
	bug("%s", "illegal execution point");
}

static int star_gentemp(struct star *star)
{
	return mtrandom_uint(stellar_clstmp[star->cls] - stellar_clstmp[star->cls + 1])
		+ stellar_clstmp[star->cls + 1];
}

static int star_genlumval(struct star *star)
{
	return mtrandom_uint(stellar_clslum[star->cls] - stellar_clslum[star->cls + 1])
		+ stellar_clslum[star->cls + 1];
}

/*
 * Parses a configuration tree and loads the information
 * into the supplied struct star*
 */
int star_load(struct star *sol, struct config *ctree)
{
	int i;
	int habset = 0;
	int tempset = 0;
	int lumset = 0;
	int lumvalset = 0;
	int clsset = 0;

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
			sscanf(ctree->data, "%u", &(sol->lumval));
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
	/* FIXME: */
	if (!lumvalset)
		sol->lumval = star_genlumval(sol);
	sol->hab = stellar_clshab[sol->cls] + stellar_lumhab[sol->lum];
	if (!tempset)
		sol->temp = star_gentemp(sol);
	sol->hablow = star_gethablow(sol->lumval);
	sol->habhigh = star_gethabhigh(sol->lumval);

	return 0;
}

static int star_generate_more(unsigned int mulodds)
{
	return (mtrandom_uint(STELLAR_CLSMUL_MAX) < mulodds);
}

unsigned long star_gethablow(unsigned int lumval)
{
	/* Rough guess looking at
	   https://secure.wikimedia.org/wikipedia/en/wiki/Habitable_zone */
	return sqrt((double)lumval/100.0) * HAB_ZONE_START * GM_PER_AU;
}

unsigned long star_gethabhigh(unsigned int lumval)
{
	return sqrt((double)lumval/100.0) * HAB_ZONE_END * GM_PER_AU;
}

static void star_init(struct star *s)
{
	s->name = NULL;
	s->cls = star_gencls();
	s->lum = star_genlum();
	s->lumval = star_genlumval(s);
	s->hab = stellar_clshab[s->cls] + stellar_lumhab[s->lum];
	s->temp = star_gentemp(s);
	s->hablow = star_gethablow(s->lum);
	s->habhigh = star_gethabhigh(s->lum);
}

#define STELLAR_MUL_MAX 4
int star_populate_system(struct system *system)
{
	struct star *sol;

	sol = malloc(sizeof(*sol));
	if (!sol)
		goto err;
	star_init(sol);

	sol->name = malloc(strlen(system->name) + 3);
	if (!sol->name)
		goto err;

	sprintf(sol->name, "%s A", system->name);
	ptrlist_push(&system->stars, sol);

	unsigned int mulodds = stellar_clsmul[sol->cls];
	for (int i = 1; star_generate_more(mulodds) && i < STELLAR_MUL_MAX; i++) {
		sol = malloc(sizeof(*sol));
		if (!sol)
			goto err;
		star_init(sol);

		sol->name = malloc(strlen(system->name) + 4);
		if (!sol->name)
			goto err;

		sprintf(sol->name, "%s %c", system->name, i + 65);
		if (stellar_clsmul[sol->cls] < mulodds)
			mulodds = stellar_clsmul[sol->cls];
		ptrlist_push(&system->stars, sol);
	}

	return 0;

err:
	if (sol && sol->name)
		free(sol->name);
	if (sol)
		free(sol);

	return -1;
}

void star_free(struct star *s)
{
	assert(s != NULL);
	free(s->name);
	free(s);
}

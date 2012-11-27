#ifndef _HAS_CIV_H
#define _HAS_CIV_H

#include "list.h"
#include "ptrlist.h"

struct universe;

struct civ {
	char* name;
	struct sector* home;
	int power;
	struct ptrlist presectors;
	struct ptrlist availnames;
	struct ptrlist sectors;
	struct list_head list;
};

struct civ* loadciv();
struct civ* civ_create();
int civ_load_all(struct civ *civs);

void civ_spawncivs(struct universe *u, struct civ *civs);
void civ_free(struct civ *civ);

#endif

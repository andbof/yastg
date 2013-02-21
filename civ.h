#ifndef _HAS_CIV_H
#define _HAS_CIV_H

#include "parseconfig.h"
#include "list.h"
#include "ptrlist.h"

struct universe;

struct civ {
	char* name;
	struct system* home;
	int power;
	struct ptrlist presystems;
	struct ptrlist availnames;
	struct ptrlist systems;
	struct ptrlist border_systems;
	struct list_head list;
	struct list_head growing;
};

void loadciv(struct civ *c, const struct list_head * const config_root);
void civ_init(struct civ *c);
int load_all_civs();

void civ_spawncivs(struct universe *u);
void civ_free(struct civ *civ);

#endif

#ifndef HAS_CIV_H
#define HAS_CIV_H

struct universe;

struct civ {
	char* name;
	struct sector* home;
	int power;
	struct ptrarray* presectors;
	struct ptrarray* availnames;
	struct ptrarray* sectors;
};

struct civ* loadciv();

struct array* loadcivs();

void civ_free(void *ptr);

void spawncivs(struct universe *u, struct array *civs);
void growciv(struct universe *u, struct civ *c);

#endif

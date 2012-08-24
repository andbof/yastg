#ifndef HAS_CIV_H
#define HAS_CIV_H

struct universe;

struct civ {
  size_t id;
  char* name;
  size_t home;
  int power;
  struct array* presectors;
  struct array* availnames;
  struct sarray* sectors;
};

struct civ* loadciv();

struct sarray* loadcivs();

void civ_free(void *ptr);

void spawncivs(struct universe *u, struct sarray *civs);
void growciv(struct universe *u, struct civ *c);

#endif

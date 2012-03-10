#ifndef HAS_STAR_H
#define HAS_STAR_H

#include "parseconfig.h"

#define STELLAR_LUM_N 9
#define STELLAR_CLS_N 7
extern const char *stellar_lum[STELLAR_LUM_N];
extern const int stellar_lumodds[STELLAR_LUM_N];
extern const int stellar_lumhab[STELLAR_LUM_N];
extern const char stellar_cls[STELLAR_CLS_N];
extern const int stellar_clsodds[STELLAR_CLS_N];
extern const unsigned long stellar_clslum[STELLAR_CLS_N+1];
extern const unsigned int stellar_clstmp[STELLAR_CLS_N+1];
extern const int stellar_clsmul[STELLAR_CLS_N];
extern const int stellar_clshab[STELLAR_CLS_N];

#define STELLAR_MUL_ODDS 4
#define STELLAR_MUL_MAX 1 // FIXME: disabled
#define STELLAR_MUL_HAB -50

struct star {
	char* name;
	int cls, lum, hab;
	unsigned long lumval, hablow, habhigh, snowline;
	unsigned int temp;
};

struct star* loadstar(struct configtree *ctree);

int star_genlum();
int star_gencls();
int star_gennum();
unsigned long star_gethablow(struct star *s);
unsigned long star_gethabhigh(struct star *s);
unsigned long star_getsnowline(struct star *s);

struct star* createstar();
struct ptrarray* createstars();
void star_free(void *ptr);

#endif

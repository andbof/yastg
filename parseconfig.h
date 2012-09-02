#ifndef _HAS_PARSECONFIG_H
#define _HAS_PARSECONFIG_H

#include <stdio.h>

struct configtree {
	char *key;
	char *data;
	struct configtree *sub;
	struct configtree *next;
};

struct configtree* parseconfig(char *fname);
struct configtree* recparseconfig(FILE *f, char *fname);
void destroyctree(struct configtree *ctree);

#endif

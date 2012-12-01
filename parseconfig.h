#ifndef _HAS_PARSECONFIG_H
#define _HAS_PARSECONFIG_H

#include <stdio.h>

struct config {
	char *key;
	char *data;
	struct config *sub;
	struct config *next;
};

struct config* parseconfig(char *fname);
struct config* recparseconfig(FILE *f, char *fname);
void destroyctree(struct config *ctree);

#endif

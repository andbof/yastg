#ifndef _HAS_PARSECONFIG_H
#define _HAS_PARSECONFIG_H

#include "list.h"

struct config {
	char *key;
	char *str;
	long l;
	struct list_head children;
	struct list_head list;
};

int parse_config_mmap(char *begin, const off_t size, struct list_head * const root);
int parse_config_file(const char * const fname, struct list_head * const root);
void destroy_config(struct list_head * const root);

#endif

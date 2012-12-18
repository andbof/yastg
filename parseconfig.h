#ifndef _HAS_PARSECONFIG_H
#define _HAS_PARSECONFIG_H

#include "list.h"

enum config_data {
	CONFIG_NONE = 0,
	CONFIG_STRING,
	CONFIG_LONG,
};

struct config {
	char *key;
	enum config_data type;
	char *str;
	long l;
	struct list_head children;
	struct list_head list;
};

int parse_config_mmap(char *begin, const off_t size, struct list_head * const root);
int parse_config_file(const char * const fname, struct list_head * const root);
void destroy_config(struct list_head * const root);

#endif

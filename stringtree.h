#ifndef _HAS_STRINGTREE_H
#define _HAS_STRINGTREE_H

#include "list.h"

/*
 * The st_node structs will look somewhat like this for the strings "test" and "tert"
 * Dashes boxes represent one st_node struct while dotted boxes is a struct list_head.
 * If a struct parameter is not listed, assume it is 0 or NULL.
 * The "->" arrows represents a linked list relationship.
 *
 * +......+  +---------+
 * . root .->| c = 't' |
 * +......+  +---------+
 *                |
 *                | children
 *                |
 *           +---------+
 *           | c = 'e' |
 *           +---------+
 *                |
 *                | children
 *                |
 *           +---------+   list    +---------+
 *           | c = 's' |---------->| c = 'r' |
 *           +---------+           +---------+
 *                |                     |
 *                | children            | children
 *                |                     |
 *           +------------------+ +----------------------+
 *           | c = 't'          | | c = 't'              |
 *           | data = a_pointer | | data = other_pointer |
 *           +------------------+ +----------------------+
 */

struct st_node {
	char c;
	void *data;
	struct list_head children;
	struct list_head list;
};

#define ST_DO_FREE_DATA 1
#define ST_DONT_FREE_DATA 0
void st_init(struct st_node * const node);
void st_destroy(struct list_head * const root, const int do_free_data);

int st_add_string(struct list_head * const root, char *string, void *data);

void* st_lookup_string(const struct list_head * const root, const char * const string);
void* st_lookup_exact(const struct list_head * const root, const char * const string);

void* st_rm_string(struct list_head * const root, const char * const string);

#endif

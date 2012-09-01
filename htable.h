#ifndef HAS_HTABLE_H
#define HAS_HTABLE_H

#include <pthread.h>
#include "list.h"

#define HTABLE_SIZE 65536

struct st_elem {
	char *key;
	void *data;
	struct list_head list;
};

struct htable {
	struct st_elem table[HTABLE_SIZE];
	pthread_rwlock_t lock;
	unsigned long elements;
};

void* htable_create();
void htable_add(struct htable *t, const char * const key, void *data);
void* htable_get(struct htable *t, const char * const key);
void htable_rm(struct htable *t, const char * const key);
void htable_free(struct htable *t);

#endif

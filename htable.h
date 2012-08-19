#ifndef HAS_HTABLE_H
#define HAS_HTABLE_H

#include <pthread.h>

#define HTABLE_SIZE 65536

struct htable {
	struct st_elem* table[HTABLE_SIZE];
	pthread_rwlock_t lock;
	unsigned long elements;
};

struct st_elem {
	char *key;
	void *data;
	struct st_elem *prev, *next;
};

void* htable_create();
void htable_add(struct htable *t, char *key, void *data);
void* htable_get(struct htable *t, char *key);
void htable_rm(struct htable *t, char *key);
void htable_free(struct htable *t);

#endif

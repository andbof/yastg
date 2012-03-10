#ifndef HAS_STABLE_H
#define HAS_STABLE_H

#include <pthread.h>

#define STABLE_SIZE 65536

struct stable {
	struct st_elem* table[STABLE_SIZE];
	pthread_mutex_t mutex;
	unsigned long elements;
};

struct st_elem {
	char *key;
	void *data;
	struct st_elem *prev, *next;
};

void* stable_create();
void stable_add(struct stable *t, char *key, void *data);
void* stable_get(struct stable *t, char *key);
void stable_rm(struct stable *t, char *key);
void stable_free(struct stable *t);

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defines.h"
#include "stable.h"
#include "log.h"

/*
 * DJBs "times 33" hash
 */
static unsigned long hash33(char *key)
{
	unsigned long hash = 5381;
	if (key == NULL)
		bug("%s", "trying to hash a null pointer");
	while (*key) {
		hash += (hash << 5) + *key;
		key++;
	}
	return hash;
}

void* stable_create()
{
	struct stable *s;
	MALLOC_DIE(s, sizeof(*s));
	memset(s, '\0', sizeof(*s));
	pthread_mutex_init(&s->mutex, NULL);
	return s;
}

void stable_add(struct stable *t, char *key, void *data)
{
	int i;
	unsigned long hash = hash33(key) % STABLE_SIZE;
	struct st_elem *prev, *cur, *next;
	pthread_mutex_lock(&t->mutex);
	cur = t->table[hash];
	if (!cur) {
		MALLOC_DIE(cur, sizeof(*cur));
		t->table[hash] = cur;
		cur->prev = NULL;
		cur->next = NULL;
	} else {
		while (cur) {
			i = strcmp(cur->data, data);
			if (i == 0) {
				bug("stable already contains element %s (address %p)", key, data);
			} else if (i < 0) {
				prev = cur;
				cur = cur->next;
				if (cur) {
					next = cur->next;
				} else {
					next = NULL;
				}
			} else if (i > 0) {
				prev = cur->prev;
				next = cur;
				cur = NULL;
			}
		}
		MALLOC_DIE(cur, sizeof(*cur));
		cur->prev = prev;
		cur->next = next;
		if (prev) {
			prev->next = cur;
		}
		if (next) {
			next->prev = cur;
		}
	}
	cur->key = key;
	cur->data = data;
	t->elements++;
	pthread_mutex_unlock(&t->mutex);
}

void* stable_get(struct stable *t, char *key)
{
	int i;
	unsigned long hash = hash33(key) % STABLE_SIZE;
	struct st_elem *elem;
	void *ptr;

	pthread_mutex_lock(&t->mutex);

	elem = t->table[hash];
	while ((elem != NULL) && ((i = strcmp(elem->key, key)) < 0))
		elem = elem->next;

	if ((elem != NULL) && (i == 0))
		ptr = (void*)elem->data;
	else
		ptr = NULL;

	pthread_mutex_unlock(&t->mutex);

	return ptr;
}

void stable_rm(struct stable *t, char *key)
{
	unsigned long hash = hash33(key) % STABLE_SIZE;
	struct st_elem *prev, *cur;

	pthread_mutex_lock(&t->mutex);

	cur = stable_get(t, key);
	if (cur == NULL)
		bug("string %s does not exist in stable", key);

	prev = cur->prev;
	if (prev == NULL) {
		free(t->table[hash]);
		t->table[hash] = NULL;
	} else {
		prev->next = cur->next;
		if (cur->next)
			cur->next->prev = prev;
		free(cur);
	}
	t->elements--;

	pthread_mutex_unlock(&t->mutex);
}

void stable_free(struct stable *t)
{
	unsigned long l;
	struct st_elem *cur;
	struct st_elem *next;

	pthread_mutex_lock(&t->mutex);

	for (l = 0; l < STABLE_SIZE; l++) {
		cur = t->table[l];
		while (cur) {
			next = cur->next;
			free(cur);
			cur = next;
		}
	}

	pthread_mutex_unlock(&t->mutex);
	pthread_mutex_destroy(&t->mutex);	/* FIXME: race condition if another thread is waiting to use t->mutex ... can we destroy a locked mutex? */

	free(t);
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "common.h"
#include "htable.h"
#include "log.h"

/*
 * DJBs "times 33" hash
 */
static unsigned long hash33(char *key)
{
	unsigned long hash = 5381;
	assert(key != NULL);
	while (*key) {
		hash += (hash << 5) + *key;
		key++;
	}
	return hash;
}

void* htable_create()
{
	struct htable *s;
	MALLOC_DIE(s, sizeof(*s));
	memset(s, '\0', sizeof(*s));
	pthread_rwlock_init(&s->lock, NULL);
	return s;
}

/* htable_add does not lock the rwlock, this is a feature */
void htable_add(struct htable *t, char *key, void *data)
{
	assert(t != NULL);
	assert(key != NULL);
	assert(data != NULL);
	int i;

	downcase_valid(key);

	unsigned long hash = hash33(key) % HTABLE_SIZE;
	struct st_elem *prev, *cur, *next;

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
				bug("key %s already exists in htable %p", key, t);
			} else if (i < 0) {
				prev = cur;
				cur = cur->next;
				if (cur)
					next = cur->next;
				else
					next = NULL;
			} else if (i > 0) {
				prev = cur->prev;
				next = cur;
				cur = NULL;
			}
		}
		MALLOC_DIE(cur, sizeof(*cur));
		cur->prev = prev;
		cur->next = next;
		if (prev)
			prev->next = cur;
		if (next)
			next->prev = cur;
	}
	cur->key = key;
	cur->data = data;
	t->elements++;
}

/* htable_get does not lock the rwlock, this is a feature */
void* htable_get(struct htable *t, char *key)
{
	assert(t != NULL);
	if (key == NULL)
		return NULL;

	int i;

	downcase_valid(key);

	unsigned long hash = hash33(key) % HTABLE_SIZE;
	struct st_elem *elem;
	void *ptr;

	elem = t->table[hash];
	while ((elem != NULL) && ((i = strcmp(elem->key, key)) < 0))
		elem = elem->next;

	if ((elem != NULL) && (i == 0))
		ptr = (void*)elem->data;
	else
		ptr = NULL;

	return ptr;
}

/* htable_rm does not lock the rwlock, this is a feature */
void htable_rm(struct htable *t, char *key)
{
	assert(t != NULL);
	assert(key != NULL);

	downcase_valid(key);

	unsigned long hash = hash33(key) % HTABLE_SIZE;
	struct st_elem *prev, *cur;

	cur = htable_get(t, key);
	if (cur == NULL)
		bug("string %s does not exist in htable", key);

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
}

void htable_free(struct htable *t)
{
	assert(t != NULL);
	unsigned long l;
	struct st_elem *cur;
	struct st_elem *next;

	pthread_rwlock_wrlock(&t->lock);

	for (l = 0; l < HTABLE_SIZE; l++) {
		cur = t->table[l];
		while (cur) {
			next = cur->next;
			free(cur);
			cur = next;
		}
	}

	pthread_rwlock_destroy(&t->lock);

	free(t);
}

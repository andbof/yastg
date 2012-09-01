#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "common.h"
#include "htable.h"
#include "log.h"
#include "list.h"

/*
 * DJBs "times 33" hash
 */
static unsigned long hash33(const char *key)
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
	for (unsigned int i = 0; i < HTABLE_SIZE; i++)
		INIT_LIST_HEAD(&s->table[i].list);
	return s;
}

/* htable_add does not lock the rwlock, this is a feature */
void htable_add(struct htable *t, const char * const key, void *data)
{
	assert(t != NULL);
	assert(key != NULL);
	assert(data != NULL);
	int i;

	char *dkey = strdup(key);
	downcase_valid(dkey);

	unsigned long hash = hash33(dkey) % HTABLE_SIZE;
	struct st_elem *pos, *list, *new;

	MALLOC_DIE(new, sizeof(*new));
	new->key = dkey;
	new->data = data;
	t->elements++;

	list_for_each_entry(pos, &t->table[hash].list, list) {
		i = strcmp(pos->key, new->key);
		if (i == 0)
			bug("key %s already exists in htable %p", key, t);
		else if (i > 0) {
			list_add_tail(&new->list, &pos->list);
			return;
		}
	}
	list_add_tail(&new->list, &pos->list);
}

static struct st_elem* _htable_get(struct htable *t, const char * key)
{
	int i;
	unsigned long hash = hash33(key) % HTABLE_SIZE;
	struct st_elem *list, *pos;
	void *ptr;

	list_for_each_entry(pos, &t->table[hash].list, list) {
		i = strcmp(pos->key, key);
		if (i == 0)
			return pos;
		else if (i > 0)
			return NULL;
	}

	return NULL;
}

/* htable_get does not lock the rwlock, this is a feature */
void* htable_get(struct htable *t, const char * const key)
{
	assert(t != NULL);
	if (key == NULL)
		return NULL;

	char *dkey = alloca(strlen(key));
	strcpy(dkey, key);
	downcase_valid(dkey);

	struct st_elem *tmp;
	tmp = _htable_get(t, dkey);

	if (tmp)
		return tmp->data;
	else
		return NULL;
}

/* htable_rm does not lock the rwlock, this is a feature */
void htable_rm(struct htable *t, const char * key)
{
	assert(t != NULL);
	assert(key != NULL);

	char *dkey = strdup(key);
	downcase_valid(dkey);

	struct st_elem *prev, *cur;

	cur = _htable_get(t, dkey);
	if (cur == NULL)
		bug("string %s does not exist in htable", key);

	list_del(&cur->list);

	free(cur->key);
	free(cur);
	t->elements--;

	free(dkey);
}

void htable_free(struct htable *t)
{
	assert(t != NULL);
	unsigned long l;
	struct st_elem *pos, *tmp;

	pthread_rwlock_wrlock(&t->lock);

	for (l = 0; l < HTABLE_SIZE; l++) {
		list_for_each_entry_safe(pos, tmp, &t->table[l].list, list) {
			list_del(&pos->list);
			free(pos->key);
			free(pos);
		}
	}

	pthread_rwlock_destroy(&t->lock);

	free(t);
}

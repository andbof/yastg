#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "common.h"
#include "list.h"
#include "log.h"
#include "ptrlist.h"
#include "sector.h"

struct ptrlist* ptrlist_init()
{
	struct ptrlist *l;
	MALLOC_DIE(l, sizeof(*l));
	INIT_LIST_HEAD(&l->list);
	pthread_rwlock_init(&l->lock, NULL);
	l->len = 0;
	l->data = NULL;
	return l;
}

void ptrlist_free(struct ptrlist *l)
{
	assert(l != NULL);
	struct ptrlist *e;
	struct list_head *p, *q;
	list_for_each_safe(p, q, &l->list) {
		e = list_entry(p, struct ptrlist, list);
		list_del(p);
		pthread_rwlock_destroy(&e->lock);
		free(e);
	}
	pthread_rwlock_destroy(&l->lock);
	free(l);
}

int ptrlist_push(struct ptrlist *l, void *e)
{
	assert(l != NULL);
	struct ptrlist *nl = ptrlist_init();
	nl->data = e;
	list_add_tail(&nl->list, &l->list);
	l->len++;
	return 0;
}

void* ptrlist_pull(struct ptrlist * const l)
{
	assert(l != NULL);
	assert(!list_empty(&l->list));
	struct list_head *head = l->list.next;
	l->len--;
	struct ptrlist *e = list_entry(head, struct ptrlist, list);
	list_del_init(head);
	void *d = e->data;
	ptrlist_free(e);
	return d;
	assert(l != NULL);
}

struct ptrlist* ptrlist_get(const struct ptrlist * const l, const unsigned long n)
{
	assert(l != NULL);
	struct list_head *ptr;
	unsigned long i = 0;
	list_for_each(ptr, &l->list) {
		if (i == n)
			break;
		i++;
	}
	return list_entry(ptr, struct ptrlist, list);
}

void* ptrlist_entry(const struct ptrlist * const l, const unsigned long n)
{
	return ptrlist_get(l, n)->data;
}

void ptrlist_rm(struct ptrlist *l, const unsigned long n)
{
	assert(l != NULL);
	assert(!list_empty(&l->list));
	l->len--;
	struct ptrlist *m = ptrlist_get(l, n);
	list_del_init(&m->list);
	ptrlist_free(m);
}

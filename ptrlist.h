#ifndef _HAS_PTRLIST_H
#define _HAS_PTRLIST_H

#include <stdio.h>
#include <pthread.h>
#include "list.h"
#include "mtrandom.h"

struct ptrlist {
	void *data;
	struct list_head list;
	pthread_rwlock_t lock;
	unsigned long len;
};

struct ptrlist* ptrlist_init();
void ptrlist_free(struct ptrlist *l);

int ptrlist_push(struct ptrlist *l, void *e);
void* ptrlist_pull(struct ptrlist * const l);
struct ptrlist* ptrlist_get(const struct ptrlist * const l, const unsigned long n);
void* ptrlist_entry(const struct ptrlist * const l, const unsigned long n);
void ptrlist_rm(struct ptrlist *l, const unsigned long n);

static inline unsigned long ptrlist_len(const struct ptrlist * const l)
{
	return l->len;
}

static inline void* ptrlist_random(const struct ptrlist * const l)
{
	return ptrlist_entry(l, mtrandom_ulong(ptrlist_len(l)));
}

#define ptrlist_data(pos)	\
	list_entry((pos), struct ptrlist, list)->data

/*
 * @iter:	variable used as iterator
 * @head:	struct list_head of the list
 * @pos:	junk list_head
 */
#define ptrlist_for_each_entry(iter, head, pos)				\
	for (pos = (head)->list.next, (iter) = ptrlist_data(pos);	\
		pos != &(head)->list;					\
		pos = pos->next, (iter) = ptrlist_data(pos))

#endif

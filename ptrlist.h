#ifndef _HAS_PTRLIST_H
#define _HAS_PTRLIST_H

#include "list.h"

struct ptrlist {
	void *data;
	struct list_head list;
	unsigned long len;
};

int ptrlist_init(struct ptrlist *l);
void ptrlist_free(struct ptrlist *l);

int ptrlist_push(struct ptrlist *l, void *e);
void* ptrlist_pull(struct ptrlist * const l);
struct ptrlist* ptrlist_get(const struct ptrlist * const l, const unsigned long n);
void* ptrlist_entry(const struct ptrlist * const l, const unsigned long n);
unsigned long ptrlist_len(const struct ptrlist * const l);
void* ptrlist_random(const struct ptrlist * const l);
void ptrlist_rm(struct ptrlist *l, const unsigned long n);
void ptrlist_sort(struct ptrlist * const l, void *data,
		int (*cmp)(const void*, const void*, void*));

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

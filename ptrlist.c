#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "common.h"
#include "list.h"
#include "mtrandom.h"
#include "ptrlist.h"

int ptrlist_init(struct ptrlist *l)
{
	memset(l, 0, sizeof(*l));

	INIT_LIST_HEAD(&l->list);

	return 0;
}

void ptrlist_free(struct ptrlist *l)
{
	assert(l != NULL);
	struct ptrlist *e, *_e;

	list_for_each_entry_safe(e, _e, &l->list, list) {
		list_del(&e->list);
		free(e);
	}

	l->len = 0;
}

int ptrlist_push(struct ptrlist *l, void *e)
{
	struct ptrlist *nl;
	assert(l != NULL);

	nl = malloc(sizeof(*nl));
	if (!nl)
		return -1;
	ptrlist_init(nl);

	nl->data = e;
	list_add_tail(&nl->list, &l->list);
	l->len++;
	return 0;
}

void* ptrlist_pull(struct ptrlist * const l)
{
	struct ptrlist *e;
	void *data;

	assert(l != NULL);

	if (list_empty(&l->list))
		return NULL;

	e = list_entry(l->list.next, struct ptrlist, list);
	list_del_init(&e->list);
	l->len--;
	data = e->data;

	free(e);

	return data;
}

struct ptrlist* ptrlist_get(const struct ptrlist * const l, const unsigned long n)
{
	assert(l != NULL);
	struct list_head *ptr;
	unsigned long i = 0;
	list_for_each(ptr, &l->list) {
		if (i == n)
			return list_entry(ptr, struct ptrlist, list);
		i++;
	}

	return NULL;
}

void* ptrlist_entry(const struct ptrlist * const l, const unsigned long n)
{
	return ptrlist_get(l, n)->data;
}

unsigned long ptrlist_len(const struct ptrlist * const l)
{
	return l->len;
}

void* ptrlist_random(const struct ptrlist * const l)
{
	if (!ptrlist_len(l))
		return NULL;

	return ptrlist_entry(l, mtrandom_ulong(ptrlist_len(l)));
}

void ptrlist_rm(struct ptrlist *l, const unsigned long n)
{
	struct ptrlist *e;

	assert(l != NULL);
	assert(!list_empty(&l->list));

	e = ptrlist_get(l, n);
	list_del(&e->list);
	free(e);

	l->len--;
}

void ptrlist_sort(struct ptrlist * const l, void *data,
		int (*cmp)(const void*, const void*, void*))
{
	const size_t len = ptrlist_len(l);
	void* list[len];

	/*
	 * This is a bit ugly, but enables us to use standard glibc functions
	 * instead of rolling our own stuff. Consider refactoring it in the
	 * future to a proper linked list sort.
	 */
	size_t n = 0;
	while (ptrlist_len(l)) {
		list[n++] = ptrlist_entry(l, 0);
		ptrlist_rm(l, 0);
	}
	qsort_r(list, len, sizeof(void*), cmp, data);

	for (n = 0; n < len; n++)
		ptrlist_push(l, list[n]);
}

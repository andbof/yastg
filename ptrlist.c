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

/*
 * Remember: the comparison function is defined as returning a value greater than,
 * equal to, or less than zero, if the first argument is respectively greather than
 * equal to, or less than the second argument.
 */

static void merge_lists(struct ptrlist *left, struct ptrlist *right,
		void *data, size_t lenl, size_t lenr,
		int (*cmp)(const void*, const void*, void*))
{
	struct ptrlist *next;
	struct ptrlist *target = list_entry(left->list.prev, struct ptrlist, list);

	while (lenl || lenr) {
		if (!lenl || (lenr && cmp(left->data, right->data, data) > 0)) {
			next = list_entry(right->list.next, struct ptrlist, list);
			list_move(&right->list, &target->list);
			target = right;
			right = next;
			lenr--;
		} else {
			next = list_entry(left->list.next, struct ptrlist, list);
			list_move(&left->list, &target->list);
			target = left;
			left = next;
			lenl--;
		}
	}
}

static void merge_chunk(struct ptrlist * const l,
		void *data, const size_t chunk,
		int (*cmp)(const void*, const void*, void*))
{
	struct ptrlist *left = ptrlist_get(l, 0);
	struct ptrlist *right = left;
	struct ptrlist *next = left;
	size_t lenl, lenr;

	while (next != l) {
		for (lenl = 0; lenl < chunk && right != l; lenl++)
			right = list_entry(right->list.next, struct ptrlist, list);

		next = right;
		for (lenr = 0; lenr < chunk && next != l; lenr++)
			next = list_entry(next->list.next, struct ptrlist, list);

		merge_lists(left, right, data, lenl, lenr, cmp);

		left = next;
		right = next;
	}
}

void ptrlist_sort(struct ptrlist * const l, void *data,
		int (*cmp)(const void*, const void*, void*))
{
	assert(l);
	const size_t len = ptrlist_len(l);

	if (len < 2)
		return;

	/*
	 * chunk < len also handles lists of lengths not evenly divisible
	 * by 2, in contrast to chunk < len / 2
	 */
	for (size_t chunk = 1; chunk < len; chunk *= 2)
		merge_chunk(l, data, chunk, cmp);
}

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "ptrlist.h"
#include "list.h"

#define NUM_TESTS 84

int cmp(const void *q, const void *p, void *data)
{
	return *(int*)q - *(int*)p;
}

static void assert_sorted(struct ptrlist * const l)
{
	struct list_head *lh;
	int *p;
	int last = -1;

	ptrlist_for_each_entry(p, l, lh) {
		assert(last + 1 == *p);
		last = *p;
	}
}

static int test_empty_lists()
{
	int tests = 0;
	struct ptrlist l;

	ptrlist_init(&l);

	ptrlist_sort(&l, NULL, cmp);
	tests++;

	assert(!ptrlist_get(&l, 0));
	tests++;

	return tests;
}

static int test_push_pull_get()
{
	int tests = 0;
	struct ptrlist l;
	int one = 1;
	int two = 2;

	ptrlist_init(&l);

	assert(!ptrlist_push(&l, &one));
	assert(*(int*)ptrlist_entry(&l, 0) == 1);
	tests++;

	assert(!ptrlist_push(&l, &two));
	assert(*(int*)ptrlist_entry(&l, 1) == 2);
	tests++;

	assert(*(int*)ptrlist_pull(&l) == 1);
	assert(*(int*)ptrlist_entry(&l, 0) == 2);
	tests++;

	ptrlist_rm(&l, 0);
	assert(!ptrlist_get(&l, 0));
	tests++;

	ptrlist_free(&l);

	return tests;
}

static int ascending(const size_t len, const int index)
{
	return index;
}

static int descending(const size_t len, const int index)
{
	return len - index - 1;
}

static int alternating(const size_t len, const int index)
{
	return (index % 2 ? index/2 : len - index/2 - 1);
}

#define LEN 13
static int test_sorting_list(int (*generate_number)(const size_t, const int))
{
	int array[LEN];
	struct ptrlist l;
	int tests = 0;

	ptrlist_init(&l);

	for (size_t len = 1; len <= LEN; len++) {

		ptrlist_init(&l);
		for (size_t i = 0; i < len; i++)
			ptrlist_push(&l, &array[i]);

		for (size_t i = 0; i < len; i++)
			array[i] = generate_number(len, i);

		ptrlist_sort(&l, NULL, cmp);
		tests++;

		assert_sorted(&l);
		tests++;

		ptrlist_free(&l);
	}

	return tests;
}

int main(int argc, char *argv[])
{
	unsigned int tests = 0;

	tests += test_empty_lists();
	tests += test_push_pull_get();
	tests += test_sorting_list(ascending);
	tests += test_sorting_list(descending);
	tests += test_sorting_list(alternating);

	assert(tests == NUM_TESTS);
}

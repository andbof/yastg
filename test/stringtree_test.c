#include <stdio.h>
#include <assert.h>
#include "stringtree.h"
#include "list.h"

#define NUM_TESTS 71

struct test_pair {
	char name[8];
	void* dest;
};

int do_tests_on_empty_head(struct list_head *root)
{
	unsigned int tests = 0;

	assert(st_lookup_string(root, "foo") == NULL);
	assert(st_rm_string(root, "foo") == NULL);
	tests += 2;

	assert(st_lookup_string(root, NULL) == NULL);
	assert(st_rm_string(root, NULL) == NULL);
	tests += 2;

	assert(st_lookup_string(root, "") == NULL);
	assert(st_rm_string(root, "") == NULL);
	tests += 2;

	assert(st_lookup_string(root, "\n") == NULL);
	assert(st_rm_string(root, "\n") == NULL);
	tests += 2;

	return tests;
}

int do_add_and_remove_tests(struct list_head *root)
{
	unsigned int tests = 0;
	int foo, bar, quz, aaabbb, aaaccc;

	assert(st_add_string(root, "foo", &foo) == 0);
	assert(st_add_string(root, "bar", &bar) == 0);
	assert(st_add_string(root, "quz", &quz) == 0);
	tests += 3;

	assert(st_lookup_string(root, "foo") == &foo);
	assert(st_lookup_string(root, "bar") == &bar);
	assert(st_lookup_string(root, "quz") == &quz);
	assert(st_lookup_string(root, "qaz") == NULL);
	tests += 4;

	assert(st_rm_string(root, "foo") == &foo);
	assert(st_rm_string(root, "bar") == &bar);
	assert(st_rm_string(root, "quz") == &quz);
	tests += 3;

	assert(st_lookup_string(root, "foo") == NULL);
	assert(st_lookup_string(root, "bar") == NULL);
	assert(st_lookup_string(root, "quz") == NULL);
	tests += 3;

	assert(st_add_string(root, "aaa bbb", &aaabbb) == 0);
	assert(st_add_string(root, "aaa ccc", &aaaccc) == 0);
	tests += 2;

	assert(st_lookup_string(root, "aaa bbb") == &aaabbb);
	assert(st_lookup_string(root, "aaa ccc") == &aaaccc);
	assert(st_lookup_string(root, "aaa") == NULL);
	assert(st_lookup_string(root, "aaa ") == NULL);
	tests += 4;

	assert(st_rm_string(root, "aaa bbb") == &aaabbb);
	assert(st_rm_string(root, "aaa ccc") == &aaaccc);
	tests += 2;

	return tests;
}

#define UNIQUENESS_PAIRS 6
int do_uniqueness_tests(struct list_head *root)
{
	unsigned int tests = 0;
	int i, j;

	int foo, fao, fzo, fzoo, foao, fooa;
	struct test_pair pairs[UNIQUENESS_PAIRS] = {
		{ "foo", &foo },
		{ "fao", &fao },
		{ "fzo", &fzo },
		{ "fzoo", &fzoo },
		{ "foao", &foao },
		{ "fooa", &fooa },
	};

	for (i = 0; i < UNIQUENESS_PAIRS; i++) {
		assert(st_add_string(root, pairs[i].name, pairs[i].dest) == 0);
		tests++;
	}

	for (i = 0; i < UNIQUENESS_PAIRS; i++) {
		assert(st_lookup_string(root, pairs[i].name) == pairs[i].dest);
		tests++;
	}

	for (i = UNIQUENESS_PAIRS - 1; i > -1; i--) {
		assert(st_rm_string(root, pairs[i].name) == pairs[i].dest);
		tests++;
		for (j = 0; j < i; j++) {
			assert(st_lookup_string(root, pairs[j].name) == pairs[j].dest);
			tests++;
		}
	}

	for (i = 0; i < UNIQUENESS_PAIRS; i++) {
		assert(st_lookup_string(root, pairs[i].name) == NULL);
		tests++;
	}

	return tests;
}

int do_destroy_tests(struct list_head *root)
{
	unsigned int tests = 0;

	st_destroy(root, 0);
	tests++;

	assert(list_empty(root));
	tests++;

	assert(st_lookup_string(root, "foo") == NULL);
	tests++;

	return tests;
}

int main(int argc, char *argv[])
{
	unsigned int tests = 0;
	LIST_HEAD(head);

	tests += do_tests_on_empty_head(&head);
	tests += do_add_and_remove_tests(&head);
	tests += do_uniqueness_tests(&head);
	tests += do_destroy_tests(&head);

	assert(tests == NUM_TESTS);

	return 0;
}

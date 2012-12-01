#include <assert.h>
#include "stringtree.h"
#include "list.h"

#define NUM_TESTS 158

struct test_pair {
	char name[32];
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

int do_shortest_match_tests(struct list_head *root)
{
	unsigned int tests = 0;

	int foo, foofoo, bar, quz, qaz;

	assert(st_add_string(root, "foo", &foo) == 0);
	assert(st_add_string(root, "foofoo", &foofoo) == 0);
	assert(st_add_string(root, "bar", &bar) == 0);
	assert(st_add_string(root, "thisisaveryloNgstring", &quz) == 0);
	assert(st_add_string(root, "thisisaveryloMgstring", &qaz) == 0);
	tests += 4;

	assert(st_lookup_string(root, "f") == NULL);
	assert(st_lookup_string(root, "fo") == NULL);
	assert(st_lookup_string(root, "foo") == &foo);
	assert(st_lookup_string(root, "foof") == &foofoo);
	assert(st_lookup_string(root, "foofo") == &foofoo);
	assert(st_lookup_string(root, "foofoo") == &foofoo);
	assert(st_lookup_string(root, "foofooo") == NULL);
	tests += 7;

	assert(st_lookup_string(root, "b") == &bar);
	assert(st_lookup_string(root, "ba") == &bar);
	assert(st_lookup_string(root, "bar") == &bar);
	assert(st_lookup_string(root, "barr") == NULL);
	assert(st_lookup_string(root, "bara") == NULL);
	tests += 5;

	assert(st_lookup_string(root, "thisisaveryloN") == &quz);
	assert(st_lookup_string(root, "thisisaveryloNg") == &quz);
	assert(st_lookup_string(root, "thisisaveryloNgs") == &quz);
	assert(st_lookup_string(root, "thisisaveryloNgst") == &quz);
	assert(st_lookup_string(root, "thisisaveryloNgstr") == &quz);
	assert(st_lookup_string(root, "thisisaveryloNgstri") == &quz);
	assert(st_lookup_string(root, "thisisaveryloNgstrin") == &quz);
	assert(st_lookup_string(root, "thisisaveryloNgstring") == &quz);
	assert(st_lookup_string(root, "thisisaveryloNgstringa") == NULL);
	tests += 10;

	assert(st_lookup_exact(root, "thisisaveryloN") == NULL);
	assert(st_lookup_exact(root, "thisisaveryloNg") == NULL);
	assert(st_lookup_exact(root, "thisisaveryloNgs") == NULL);
	assert(st_lookup_exact(root, "thisisaveryloNgst") == NULL);
	assert(st_lookup_exact(root, "thisisaveryloNgstr") == NULL);
	assert(st_lookup_exact(root, "thisisaveryloNgstri") == NULL);
	assert(st_lookup_exact(root, "thisisaveryloNgstrin") == NULL);
	assert(st_lookup_exact(root, "thisisaveryloNgstring") == &quz);
	assert(st_lookup_exact(root, "thisisaveryloNgstringa") == NULL);
	tests += 10;

	assert(st_lookup_string(root, "thisisaveryloM") == &qaz);
	assert(st_lookup_string(root, "thisisaveryloMg") == &qaz);
	assert(st_lookup_string(root, "thisisaveryloMgs") == &qaz);
	assert(st_lookup_string(root, "thisisaveryloMgst") == &qaz);
	assert(st_lookup_string(root, "thisisaveryloMgstr") == &qaz);
	assert(st_lookup_string(root, "thisisaveryloMgstri") == &qaz);
	assert(st_lookup_string(root, "thisisaveryloMgstrin") == &qaz);
	assert(st_lookup_string(root, "thisisaveryloMgstring") == &qaz);
	assert(st_lookup_string(root, "thisisaveryloMgstringa") == NULL);
	tests += 10;

	assert(st_lookup_exact(root, "thisisaveryloM") == NULL);
	assert(st_lookup_exact(root, "thisisaveryloMg") == NULL);
	assert(st_lookup_exact(root, "thisisaveryloMgs") == NULL);
	assert(st_lookup_exact(root, "thisisaveryloMgst") == NULL);
	assert(st_lookup_exact(root, "thisisaveryloMgstr") == NULL);
	assert(st_lookup_exact(root, "thisisaveryloMgstri") == NULL);
	assert(st_lookup_exact(root, "thisisaveryloMgstrin") == NULL);
	assert(st_lookup_exact(root, "thisisaveryloMgstring") == &qaz);
	assert(st_lookup_exact(root, "thisisaveryloMgstringa") == NULL);
	tests += 10;

	assert(st_lookup_string(root, "t") == NULL);
	assert(st_lookup_string(root, "th") == NULL);
	assert(st_lookup_string(root, "thi") == NULL);
	assert(st_lookup_string(root, "this") == NULL);
	assert(st_lookup_string(root, "thisi") == NULL);
	assert(st_lookup_string(root, "thisis") == NULL);
	assert(st_lookup_string(root, "thisisa") == NULL);
	assert(st_lookup_string(root, "thisisav") == NULL);
	assert(st_lookup_string(root, "thisisave") == NULL);
	assert(st_lookup_string(root, "thisisaver") == NULL);
	assert(st_lookup_string(root, "thisisavery") == NULL);
	assert(st_lookup_string(root, "thisisaveryl") == NULL);
	assert(st_lookup_string(root, "thisisaverylo") == NULL);
	tests += 13;

	assert(st_lookup_exact(root, "t") == NULL);
	assert(st_lookup_exact(root, "th") == NULL);
	assert(st_lookup_exact(root, "thi") == NULL);
	assert(st_lookup_exact(root, "this") == NULL);
	assert(st_lookup_exact(root, "thisi") == NULL);
	assert(st_lookup_exact(root, "thisis") == NULL);
	assert(st_lookup_exact(root, "thisisa") == NULL);
	assert(st_lookup_exact(root, "thisisav") == NULL);
	assert(st_lookup_exact(root, "thisisave") == NULL);
	assert(st_lookup_exact(root, "thisisaver") == NULL);
	assert(st_lookup_exact(root, "thisisavery") == NULL);
	assert(st_lookup_exact(root, "thisisaveryl") == NULL);
	assert(st_lookup_exact(root, "thisisaverylo") == NULL);
	tests += 13;

	return tests;
}

int do_case_insensitive_tests(struct list_head *root)
{
	unsigned int tests = 0;

	int foo;

	assert(st_add_string(root, "foo", &foo) == 0);
	tests++;

	assert(st_lookup_string(root, "foo") == &foo);
	tests++;

	assert(st_lookup_string(root, "fOo") == &foo);
	tests++;

	assert(st_rm_string(root, "foo") == &foo);
	tests++;

	assert(st_rm_string(root, "fOo") == NULL);
	tests++;

	return tests;
}

int do_destroy_tests(struct list_head *root)
{
	unsigned int tests = 0;

	st_destroy(root, ST_DONT_FREE_DATA);
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
	tests += do_shortest_match_tests(&head);
	tests += do_case_insensitive_tests(&head);
	tests += do_destroy_tests(&head);

	assert(tests == NUM_TESTS);

	return 0;
}

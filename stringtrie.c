#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stringtrie.h>
#include "common.h"
#include "crunch.h"

/*
 * The array lengths of eight bytes come from the 6-bit crunched representation
 * of characters in the trie: 2**6 = 64 = 8 * 8
 */
struct st_lsb {
	struct st_root *lsb[8];
};

void st_init(struct st_root * const root)
{
	assert(root);
	memset(root, 0, sizeof(*root));
}

static unsigned char get_char_lsb(const char c)
{
	return crunch(c) & 0x07;
}

static unsigned char get_char_msb(const char c)
{
	return (crunch(c) >> 3) & 0x7;
}

static void st_destroy_root(struct st_root *root,
		const enum st_free_data do_free_data)
{
	struct st_lsb *lsb;
	struct st_root *next;

	if (do_free_data)
		free(root->data);

	root->data = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(root->msb); i++) {
		lsb = root->msb[i];
		if (!lsb)
			continue;
		root->msb[i] = NULL;

		for (size_t j = 0; j < ARRAY_SIZE(lsb->lsb); j++) {
			next = lsb->lsb[j];
			if (!next)
				continue;

			lsb->lsb[j] = NULL;
			st_destroy_root(next, do_free_data);
			free(next);
		}

		free(lsb);
	}
}

void st_destroy(struct st_root * const root,
		const enum st_free_data do_free_data)
{
	assert(root);
	assert(do_free_data == ST_DO_FREE_DATA || do_free_data == ST_DONT_FREE_DATA);

	st_destroy_root(root, do_free_data);
}

int st_add_string(struct st_root * const root, const char *string, void *data)
{
	assert(root);

	unsigned char c_lsb, c_msb;
	struct st_lsb *lsb;
	struct st_root *node;

	if (!string || !string[0])
		return -1;

	node = root;

	for (const char *c = string; *c; c++) {
		c_lsb = get_char_lsb(*c);
		c_msb = get_char_msb(*c);

		if (!node->msb[c_msb]) {
			node->msb[c_msb] = malloc(sizeof(*node->msb[c_msb]));
			memset(node->msb[c_msb], 0, sizeof(*node->msb[c_msb]));
		}
		lsb = node->msb[c_msb];
		if (!lsb->lsb[c_lsb]) {
			lsb->lsb[c_lsb] = malloc(sizeof(*lsb->lsb[c_lsb]));
			memset(lsb->lsb[c_lsb], 0, sizeof(*lsb->lsb[c_lsb]));
		}
		node = lsb->lsb[c_lsb];
	}

	node->data = data;

	return 0;
}

/*
 * If there is only one node with a data pointer if we traverse all child nodes
 * from root, function get_the_only_child() will return the data pointer for
 * that node. Otherwise, it will return NULL. This is useful for implementing
 * matching the shortest unique string for a set of strings.
 */
static const struct st_root *get_the_only_child(
		const struct st_root * const root)
{
	struct st_root *node = NULL;
	struct st_lsb *lsb = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(root->msb); i++) {
		if (!root->msb[i])
			continue;

		if (lsb)
			return NULL;

		lsb = root->msb[i];
	}

	if (root->data) {
		if (lsb)
			return NULL;

		return root;
	}

	for (size_t i = 0; i < ARRAY_SIZE(lsb->lsb); i++) {
		if (!lsb->lsb[i])
			continue;

		if (node)
			return NULL;

		node = lsb->lsb[i];
	}

	/*
	 * This should never fail because it's impossible to have an msb
	 * without an lsb. Remember: the data pointer is in the msb.
	 */
	assert(node);

	return get_the_only_child(node);
}

static const struct st_root *find_node(const struct st_root * const root,
		const char * const string, int exact_match_only)
{
	unsigned char c_lsb, c_msb;
	struct st_lsb *lsb;
	const struct st_root *node = root;

	for (const char *c = string; *c; c++) {
		c_lsb = get_char_lsb(*c);
		c_msb = get_char_msb(*c);

		lsb = node->msb[c_msb];
		if (!lsb)
			return NULL;

		node = lsb->lsb[c_lsb];
		if (!node)
			return NULL;
	}

	if (node->data || exact_match_only)
		return node;
	else
		return get_the_only_child(node);
}

void *st_lookup_string(const struct st_root * const root,
		const char * const string)
{
	const struct st_root *node;

	if (!root || !string || !*string)
		return NULL;

	node = find_node(root, string, 0);
	if (!node)
		return NULL;

	return node->data;
}


void *st_lookup_exact(const struct st_root * const root,
		const char * const string)
{
	const struct st_root *node;

	if (!root || !string || !*string)
		return NULL;

	node = find_node(root, string, 1);
	if (!node)
		return NULL;

	return node->data;
}

static int is_msb_free(const struct st_root *root)
{
	for (size_t i = 0; i < ARRAY_SIZE(root->msb); i++) {
		if (root->msb[i])
			return 0;
	}

	return 1;
}

static int is_lsb_free(const struct st_lsb *lsb)
{
	for (size_t i = 0; i < ARRAY_SIZE(lsb->lsb); i++) {
		if (lsb->lsb[i])
			return 0;
	}

	return 1;
}

void *st_rm_string(struct st_root * const root, const char * const string)
{
	struct pair {
		struct st_root *root;
		struct st_lsb *lsb;
	};

	unsigned char c_lsb, c_msb;
	struct st_lsb *lsb;
	struct st_root *node = root;

	if (!root || !string || !string[0])
		return NULL;

	const size_t str_len = strlen(string);
	struct pair path[str_len];

	for (size_t i = 0; string[i]; i++) {
		c_lsb = get_char_lsb(string[i]);
		c_msb = get_char_msb(string[i]);

		lsb = node->msb[c_msb];
		if (!lsb)
			return NULL;

		path[i].root = node;

		node = lsb->lsb[c_lsb];
		if (!node)
			return NULL;

		path[i].lsb = lsb;
	}

	void * const data = node->data;
	node->data = NULL;

	if (!is_msb_free(node))
		return data;

	for (size_t i = str_len - 1; i > 0; i--) {
		lsb = path[i].lsb;
		if (lsb->lsb[get_char_lsb(string[i])]->data)
			return data;

		free(lsb->lsb[get_char_lsb(string[i])]);
		lsb->lsb[get_char_lsb(string[i])] = NULL;
		if (!is_lsb_free(lsb))
			return data;

		node = path[i].root;
		free(node->msb[get_char_msb(string[i])]);
		node->msb[get_char_msb(string[i])] = NULL;
		if (!is_msb_free(node))
			return data;
	}

	return data;
}

int st_is_empty(const struct st_root * const root)
{
	if (!is_msb_free(root))
		return 0;

	return 1;
}

static char get_char_from_msb_lsb(unsigned char msb, unsigned char lsb)
{
	return decrunch((msb << 3) | lsb);
}

void __st_foreach_data(struct st_root *root,
		void (*func)(void *data, const char *string, void *hints),
		void *hints, char *buf, size_t idx, size_t len)
{
	struct st_lsb *lsb;
	struct st_root *next;

	if (idx >= len)
		return;

	if (root->data) {
		buf[idx] = '\0';
		func(root->data, buf, hints);
	}

	for (size_t i = 0; i < ARRAY_SIZE(root->msb); i++) {
		lsb = root->msb[i];
		if (!lsb)
			continue;

		for (size_t j = 0; j < ARRAY_SIZE(lsb->lsb); j++) {
			next = lsb->lsb[j];
			if (!next)
				continue;

			buf[idx] = get_char_from_msb_lsb(i, j);
			__st_foreach_data(next, func, hints, buf, idx + 1, len);
		}
	}
}

void st_foreach_data(struct st_root *root,
		void (*func)(void *data, const char *string, void *hints),
		void *hints)
{
	char buf[64];

	return __st_foreach_data(root, func, hints, buf, 0, ARRAY_SIZE(buf));
}

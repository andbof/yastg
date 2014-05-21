#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stringtree.h>
#include "common.h"
#include "crunch.h"
#include "list.h"

struct char_list {
	char c;
	struct list_head list;
};

void st_init(struct st_node * const node)
{
	assert(node);

	memset(node, 0, sizeof(*node));

	INIT_LIST_HEAD(&node->children);
	INIT_LIST_HEAD(&node->list);
}

void st_destroy(struct list_head * const root, const enum st_free_data do_free_data)
{
	struct st_node *node, *_node;

	assert(root);

	list_for_each_entry_safe(node, _node, root, list) {
		if (!list_empty(&node->children))
			st_destroy(&node->children, do_free_data);
		list_del(&node->list);
		if (do_free_data == ST_DO_FREE_DATA)
			free(node->data);
		free(node);
	}
}

static int _st_add_string(struct list_head * const root, const char *string,
		void *data)
{
	struct st_node *new_node, *prev_node;
	char c = crunch(string[0]);

	new_node = NULL;
	list_for_each_entry(prev_node, root, list) {
		if (prev_node->c == c)
			new_node = prev_node;
		if (prev_node->c >= c)
			break;
	}

	if (!new_node) {
		new_node = malloc(sizeof(*new_node));
		if (!new_node)
			return -1;

		st_init(new_node);
		new_node->c = c;

		list_add_tail(&new_node->list, &prev_node->list);
	}

	if (string[1] == '\0') {
		new_node->data = data;
		return 0;
	} else {
		return st_add_string(&new_node->children, string + 1, data);
	}
}

int st_add_string(struct list_head * const root, const char *string, void *data)
{
	int r;

	if (!root || !string || string[0] == '\0')
		return -1;

	r = _st_add_string(root, string, data);

	return r;
}

/*
 * If there is only one node with a data pointer if we traverse all child nodes
 * from root, function get_the_only_child() will return that node. Otherwise,
 * it will return NULL. This is useful for implementing matching the shortest
 * unique string for a set of strings.
 */
static int _get_the_only_child(const struct list_head * const root, struct st_node **unique)
{
	struct st_node *node;
	int r;

	list_for_each_entry(node, root, list) {
		if (node->data) {
			if (!*unique)
				*unique = node;
			else
				return -1;
		}
		if (!list_empty(&node->children)) {
			r = _get_the_only_child(&node->children, unique);
			if (r)
				return r;
		}
	}

	return 0;
}

static void* get_the_only_child(const struct list_head * const root)
{
	struct st_node *node = NULL;
	int r;

	r = _get_the_only_child(root, &node);
	if (r)
		return NULL;

	return node;
}

static struct st_node* find_node(const struct list_head * const root, const char * const string, const int exact_match_only)
{
	struct st_node *node;
	char c = crunch(string[0]);

	list_for_each_entry(node, root, list) {
		if (node->c < c)
			continue;

		if (node->c > c)
			return NULL;

		if (string[1] != '\0')
			return find_node(&node->children, string + 1, exact_match_only);

		if (node->data)
			return node;

		if (exact_match_only)
			return NULL;
		else
			return get_the_only_child(&node->children);
	}

	return NULL;
}

void* st_lookup_string(const struct list_head * const root, const char * const string)
{
	struct st_node *node;

	if (!root || !string || string[0] == '\0')
		return NULL;

	node = find_node(root, string, 0);
	if (!node)
		return NULL;

	return node->data;
}


void* st_lookup_exact(const struct list_head * const root, const char * const string)
{
	struct st_node *node;

	if (!root || !string || string[0] == '\0')
		return NULL;

	node = find_node(root, string, 1);
	if (!node)
		return NULL;

	return node->data;
}

/*
 * st_rm_string removes a string from the string tree by setting the data
 * pointer for the string to NULL, but does not deallocate the tree nodes.
 * This is a feature for trees where it is likely the nodes will be reused
 * later on and we want to keep the calls to malloc() and free() down.
 */
void* st_rm_string(struct list_head * const root, const char * const string)
{
	void *data;
	struct st_node *node;

	if (!root || !string || string[0] == '\0')
		return NULL;

	node = find_node(root, string, 1);
	if (!node)
		return NULL;

	data = node->data;
	node->data = NULL;

	return data;
}

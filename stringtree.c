#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stringtree.h>
#include "common.h"

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

void st_destroy(struct list_head * const root, const int do_free_data)
{
	struct st_node *node, *_node;

	assert(root);

	list_for_each_entry_safe(node, _node, root, list) {
		if (!list_empty(&node->children))
			st_destroy(&node->children, do_free_data);
		list_del(&node->list);
		if (do_free_data)
			free(node->data);
		free(node);
	}
}

static int _st_add_string(struct list_head * const root, char *string, void *data)
{
	struct st_node *new_node, *prev_node;

	new_node = NULL;
	list_for_each_entry(prev_node, root, list) {
		if (prev_node->c == string[0])
			new_node = prev_node;
		if (prev_node->c >= string[0])
			break;
	}

	if (!new_node) {
		new_node = malloc(sizeof(*new_node));
		if (!new_node)
			return -1;

		st_init(new_node);
		new_node->c = string[0];

		list_add_tail(&new_node->list, &prev_node->list);
	}

	if (string[1] == '\0') {
		new_node->data = data;
		return 0;
	} else {
		return st_add_string(&new_node->children, string + 1, data);
	}
}

int st_add_string(struct list_head * const root, char *_string, void *data)
{
	char *string;
	int r;

	if (!root || !_string || _string[0] == '\0')
		return -1;

	string = strdup(_string);
	if (!string)
		return -1;

	downcase_valid(string);

	r = _st_add_string(root, string, data);

	free(string);
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
	struct st_node *node, *unique;
	int i;

	list_for_each_entry(node, root, list) {
		if (node->c < string[0])
			continue;

		if (node->c > string[0])
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

void* st_lookup_string(const struct list_head * const root, const char * const _string)
{
	struct st_node *node;
	char *string;

	if (!root || !_string || _string[0] == '\0')
		return NULL;

	string = strdup(_string);
	if (!string)
		return NULL;

	downcase_valid(string);

	node = find_node(root, string, 0);

	free(string);
	if (!node)
		return NULL;

	return node->data;
}


void* st_lookup_exact(const struct list_head * const root, const char * const _string)
{
	struct st_node *node;
	char *string;

	if (!root || !_string || _string[0] == '\0')
		return NULL;

	string = strdup(_string);
	if (!string)
		return NULL;

	downcase_valid(string);

	node = find_node(root, string, 1);

	free(string);
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
void* st_rm_string(struct list_head * const root, const char * const _string)
{
	void *data;
	struct st_node *node;
	char *string;

	if (!root || !_string || _string[0] == '\0')
		return NULL;

	string = strdup(_string);
	if (!string)
		return NULL;

	downcase_valid(string);

	node = find_node(root, string, 1);

	free(string);
	if (!node)
		return NULL;

	data = node->data;
	node->data = NULL;

	return data;
}

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stringtree.h>

struct char_list {
	char c;
	struct list_head list;
};

void st_init(struct st_node * const node)
{
	assert(node != NULL);

	memset(node, 0, sizeof(*node));

	INIT_LIST_HEAD(&node->children);
	INIT_LIST_HEAD(&node->list);
}

void st_destroy(struct list_head * const root, const int do_free_data)
{
	struct st_node *node, *_node;
	list_for_each_entry_safe(node, _node, root, list) {
		if (!list_empty(&node->children))
			st_destroy(&node->children, do_free_data);
		list_del(&node->list);
		if (do_free_data)
			free(node->data);
		free(node);
	}
}

int st_add_string(struct list_head * const root, char *string, void *data)
{
	struct st_node *new_node, *prev_node;

	if (root == NULL || string[0] == '\0')
		return -1;

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

static struct st_node* find_node(const struct list_head * const root, const char * const string)
{
	assert(root != NULL);
	struct st_node *node;

	if (!string || string[0] == '\0')
		return NULL;

	list_for_each_entry(node, root, list) {
		if (node->c < string[0])
			continue;

		if (node->c > string[0])
			return NULL;

		if (string[1] != '\0')
			return find_node(&node->children, string + 1);

		if (node->data)
			return node;

		return NULL;
	}

	return NULL;
}

void* st_lookup_string(const struct list_head * const root, const char * const string)
{
	struct st_node *node;

	node = find_node(root, string);
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

	node = find_node(root, string);
	if (!node)
		return NULL;

	data = node->data;
	node->data = NULL;

	return data;
}

#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include "parseconfig.h"
#include "list.h"
#include "log.h"

char test_data[] =
	"#comment at start of file\n"
	"key \"this is a data string\"\n"
	"key2 anotherdatastring\n"
	"   key3   \"whitespace test\"   \n"
	"	key4		\"tab test\"		\n"
	"# This is a comment and the following lines are empty and/or contain just whitespace\n"
	"\n"
	"	\n"
	"  \n"
	"	  \n"
	"key5 \"data string\" {\n"
	"	key5.1 \"is a child of key5\"\n"
	"	key5.2 \"is another child\"	# a comment at the end of a line\n"
	"	key5.3 \"is the next child\"\n"
	"}\n"
	"key6 {\n"
	"	key6.1 \"key6 has no data\"\n"
	"}\n"
	"key7\n"
	"#comment at end of file\n"
	"#another comment at the end of file"
	"\0";

#define NUM_TESTS 61
#define NUM_CONFIG_KEYS 7

static unsigned int verify_comments(const struct list_head * const root)
{
	unsigned int tests = 0;
	struct config *config;

	list_for_each_entry(config, root, list) {
		assert(strchr(config->key, '#') == NULL);
		assert(strstr(config->key, "comment") == NULL);
		tests++;

		if (config->data) {
			assert(strchr(config->data, '#') == NULL);
			assert(strstr(config->data, "comment") == NULL);
			tests++;
		}

		tests += verify_comments(&config->children);

	}

	return tests;
}

static unsigned int verify_no_empty_rows(const struct list_head * const root)
{
	unsigned int tests = 0;
	struct config *config;

	list_for_each_entry(config, root, list) {
		assert(strlen(config->key) > 0);
		tests++;

		tests += verify_no_empty_rows(&config->children);
	}

	return tests;
}

static unsigned int verify_whitespace(const struct list_head * const root)
{
	unsigned int tests = 0;
	struct config *config;

	list_for_each_entry(config, root, list) {
		assert(!isspace(config->key[0]));
		assert(!isspace(config->key[strlen(config->key) - 1]));
		tests++;

		if (config->data) {
			assert(!isspace(config->data[0]));
			assert(!isspace(config->data[strlen(config->data) - 1]));
			tests++;
		}

		tests += verify_whitespace(&config->children);
	}

	return tests;
}

static unsigned int verify_children(const struct list_head * const root)
{
	unsigned int tests = 0;
	struct config *config;
	int children[NUM_CONFIG_KEYS] = {0, 0, 0, 0, 3, 1, 0};

	list_for_each_entry(config, root, list) {
		assert(list_len(&config->children) == children[tests]);
		tests++;
	}

	return tests;
}

int main(int argc, char* argv[])
{
	unsigned int tests = 0;
	struct list_head root;
	INIT_LIST_HEAD(&root);

	log_init_stdout();

	assert(!parse_config_mmap(test_data, sizeof(test_data), &root));
	tests++;

	assert(list_len(&root) == NUM_CONFIG_KEYS);
	tests++;

	tests += verify_comments(&root);
	tests += verify_no_empty_rows(&root);
	tests += verify_whitespace(&root);
	tests += verify_children(&root);

	destroy_config(&root);
	tests++;

	assert(tests == NUM_TESTS);

	log_close();

	return 0;
}

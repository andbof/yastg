#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
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
	"hex 0x128\n"
	"dec 128\n"
	"sci 128e3\n"
	"neg -64\n"
	"negsci -64e3\n"
	"maxsci  2e400\n"
	"max     91385098193531095109350931531656587632875687326587465874369874368764876\n"
	"min    -98739579831365187365873165315876318756318757316875631875631876587136587\n"
	"minsci -2e400\n"
	"#comment at end of file\n"
	"#another comment at the end of file"
	"\0";

#define NUM_TESTS 113
#define NUM_CONFIG_KEYS 16

static unsigned int verify_comments(const struct list_head * const root)
{
	unsigned int tests = 0;
	struct config *config;

	list_for_each_entry(config, root, list) {
		assert(strchr(config->key, '#') == NULL);
		assert(strstr(config->key, "comment") == NULL);
		tests++;

		if (config->str) {
			assert(strchr(config->str, '#') == NULL);
			assert(strstr(config->str, "comment") == NULL);
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

		if (config->str) {
			assert(!isspace(config->str[0]));
			assert(!isspace(config->str[strlen(config->str) - 1]));
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
	int children[NUM_CONFIG_KEYS] = {0, 0, 0, 0, 3, 1, 0, 0, 0, 0, 0, 0};

	list_for_each_entry(config, root, list) {
		assert(list_len(&config->children) == children[tests]);
		tests++;
	}

	return tests;
}

static unsigned int verify_data(const struct list_head * const root)
{
	unsigned int tests = 0;
	struct config *config;

	char *keys[NUM_CONFIG_KEYS] = {
		"key",		"key2",		"key3",		"key4",
		"key5",		"key6",		"key7",		"hex",
		"dec",		"sci",		"neg",		"negsci",
		"maxsci",	"max",		"min",		"minsci",
	};


	char *strings[NUM_CONFIG_KEYS] = {
		"this is a data string",	"anotherdatastring",
		"whitespace test",		"tab test",
		"data string",
		NULL,		NULL,		NULL,		NULL,
		NULL,		NULL,		NULL,		NULL,
		NULL,		NULL,		NULL,
	};


	long numbers[NUM_CONFIG_KEYS] = {
		0,		0,		0,		0,
		0,		0,		0,		0x128,
		128,		128000,		-64,		-64000,
		LONG_MAX,	LONG_MAX,	LONG_MIN,	LONG_MIN,
	};

	list_for_each_entry(config, root, list) {
		assert(!strcmp(config->key, keys[tests]));

		if (strings[tests])
			assert(!strcmp(config->str, strings[tests]));
		else
			assert(config->str == NULL);

		assert(config->l == numbers[tests]);

		tests++;
	}

	return tests;
}

int main(int argc, char *argv[])
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
	tests += verify_data(&root);

	destroy_config(&root);
	tests++;

	assert(tests == NUM_TESTS);

	log_close();

	return 0;
}

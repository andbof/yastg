#include <assert.h>
#include <limits.h>
#include <string.h>
#include "cli.h"
#include "list.h"

#define NUM_TESTS 57

struct test_data {
	const char cmd[32];
	int (*func)(void*, char*);
	const char *help;
	void *data;
	int ret;
};

static int return_one(void *data, char *string)
{
	return 1;
}
char return_one_help[] = "Always returns one";

static int return_two(void *data, char *string)
{
	return 2;
}
char return_two_help[] = "Always returns two";

static int return_int(void *_data, char *string)
{
	int *data = _data;
	return *data;
}
char return_int_help[] = "Always returns *(int*)data";

static int string_is_valid(void *data, char *string)
{
	if (!string)
		return -1;

	if (strcmp(string, "valid") == 0)
		return 0;
	else
		return -1;
}
char string_is_valid_help[] = "Returns 0 if string is 'valid', else -1";

/*
 * Try to add commands only consisting of invalid characters
 * in all lengths from one character up to INVALID_MAX_LENGTH.
 */
#define INVALID_MAX_LENGTH 3
static int do_add_invalid_cmds_test(struct st_root *head)
{
	unsigned int tests = 0;
	int data = 0;
	char string[INVALID_MAX_LENGTH + 1];
	char chars[] = { '\0', ' ', '\t', '\n', '\r' };

	for (size_t t = 0; t < ARRAY_SIZE(chars); t++) {
		memset(string, '\0', sizeof(string));

		for (size_t i = 0; i < INVALID_MAX_LENGTH; i++) {
			for (size_t j = 0; j <= i; j++)
				string[j] = chars[t];

			assert(cli_add_cmd(head, string, &return_one, &data, return_one_help) < 0);
			tests++;
		}
	}

	return tests;
}

static int do_add_remove_tests(struct st_root *head)
{
	unsigned int tests = 0;
	int data = 0;
	char strings[][5] = { "foo", "fo", "bar", "quz", "quuz" };

	for (size_t i = 0; i < ARRAY_SIZE(strings); i++) {
		assert(!cli_add_cmd(head, strings[i], &return_one, &data, return_one_help));
		for (size_t j = 0; j <= i; j++)
			assert(cli_run_cmd(head, strings[j]) == 1);

		tests++;
	}

	for (size_t i = 0; i < ARRAY_SIZE(strings); i++) {
		assert(!cli_rm_cmd(head, strings[i]));
		for (size_t j = 0; j <= i; j++)
			assert(cli_run_cmd(head, strings[j]) < 0);

		for (size_t j = i + 1; j < ARRAY_SIZE(strings); j++)
			assert(cli_run_cmd(head, strings[j]) == 1);

		tests++;
	}

	return tests;
}

static int do_uniqueness_tests(struct st_root *head)
{
	unsigned int tests = 0;
	int data = 0;

	assert(!cli_add_cmd(head, "f", &return_one, &data, return_one_help));
	assert(!cli_add_cmd(head, "fo", &return_two, &data, return_two_help));
	tests += 2;

	assert(cli_run_cmd(head, "f") == 1);
	assert(cli_run_cmd(head, "fo") == 2);
	tests += 2;

	assert(!cli_rm_cmd(head, "f"));
	assert(cli_run_cmd(head, "f") == 2);
	assert(cli_run_cmd(head, "fo") == 2);
	tests += 3;

	return tests;
}

static int do_data_tests(struct st_root *head)
{
	unsigned int tests = 0;
	int data = 0;
	int codes[] = { INT_MIN, INT_MIN + 1, -1, 0, 1, INT_MAX - 1, INT_MAX };

	assert(!cli_add_cmd(head, "foo", &return_int, &data, return_int_help));
	tests++;

	for (size_t i = 0; i < ARRAY_SIZE(codes); i++) {
		data = codes[i];
		assert(cli_run_cmd(head, "foo") == data);
		tests++;
	}

	assert(!cli_rm_cmd(head, "foo"));
	tests++;

	return tests;
}

static int do_param_tests(struct st_root *head)
{
	unsigned int tests = 0;

	assert(!cli_add_cmd(head, "foo", &string_is_valid, NULL, string_is_valid_help));
	tests++;

	assert(cli_run_cmd(head, "foo"));
	assert(cli_run_cmd(head, "foovalid"));
	tests += 2;

	assert(!cli_run_cmd(head, "foo valid"));
	assert(!cli_run_cmd(head, "foo valid "));
	assert(!cli_run_cmd(head, "foo  valid "));
	tests += 3;

	assert(!cli_run_cmd(head, "foo\tvalid"));
	assert(!cli_run_cmd(head, "foo\tvalid\t"));
	assert(!cli_run_cmd(head, "foo\t\tvalid\t"));
	tests += 3;

	assert(!cli_rm_cmd(head, "foo"));
	tests++;

	return tests;
}

static int do_run_invalid_cmds_test(struct st_root *head)
{
	unsigned int tests = 0;

	assert(!cli_add_cmd(head, "foo", &return_one, NULL, return_one_help));
	tests++;

	assert(cli_run_cmd(head, NULL) < 0);
	assert(cli_run_cmd(head, "") < 0);
	tests += 2;

	assert(cli_run_cmd(head, " ") < 0);
	assert(cli_run_cmd(head, "\t") < 0);
	tests += 2;

	assert(!cli_rm_cmd(head, "foo"));
	tests++;

	return tests;
}

int main(int argc, char *argv[])
{
	unsigned int tests = 0;
	struct st_root head;
	st_init(&head);

	tests += do_add_invalid_cmds_test(&head);
	tests += do_add_remove_tests(&head);
	tests += do_uniqueness_tests(&head);
	tests += do_data_tests(&head);
	tests += do_param_tests(&head);
	tests += do_run_invalid_cmds_test(&head);

	assert(tests == NUM_TESTS);

	return 0;
}

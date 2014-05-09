#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "crunch.h"

#define NUM_TESTS 525

/*
 * This might seem like a stupid test case, but it's so easy to get the table
 * in crunch.c wrong. Better safe than sorry.
 */

static void uncrunch(char s[], const size_t len)
{
	for (size_t i = 0; i < len; i++)
		s[i] += 32;
}

static int is_valid_crunched(const char c)
{
	/* 0xc0 = bit 6 and 7 set */
	if (c & 0xc0)
		return 0;

	return 1;
}

static int is_valid_uncrunched(const char c)
{
	if (c < ' ' || c > '_')
		return 0;

	return 1;
}

static size_t test_problematic_chars(void)
{
	struct test_pair {
		char in[2];
		char out[2];
	};
	struct test_pair *pair;
	static struct test_pair pairs[] = {
		{ "a",	  "A" }, { "z",    "Z" }, { "A",    "A" },
		{ "Z",    "Z" }, { "0",	   "0" }, { "9",    "9" },
		{ " ",    " " }, { "_",    "_" }, { "\x1",  "_" },
		{ "\xff", "_" }, { "\x1f", "_" }, { "\x60", "_" },
		{"\0","\0"}
	};
	size_t tests = 0;
	char *out;

	for (pair = pairs; *pair->in; pair++) {
		out = duplicate_and_crunch(pair->in);
		uncrunch(out, strlen(pair->in));
		assert(!strcmp(out, pair->out));
		free(out);
		tests++;
	}

	return tests;
}

int main()
{
	size_t tests = 0;
	char ascii[UINT8_MAX + 1];
	char *out;

	for (int i = 0; i < sizeof(ascii) - 1; i++)
		ascii[i] = i + 1;
	ascii[sizeof(ascii) - 1] = '\0';
	assert(strlen(ascii) == sizeof(ascii) - 1);

	out = duplicate_and_crunch(ascii);
	tests++;

	for (int i = 0; i < sizeof(ascii); i++) {
		assert(is_valid_crunched(out[i]));
		tests++;
	}

	uncrunch(out, sizeof(ascii) - 1);

	assert(strlen(out) == sizeof(ascii) - 1);
	tests++;

	for (char *c = out; *c; c++) {
		assert(is_valid_uncrunched(*c));
		tests++;
	}

	tests += test_problematic_chars();

	free(out);
	assert(tests == NUM_TESTS);
}

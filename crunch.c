#include <stdlib.h>
#include <string.h>
#include "crunch.h"

/*
 * Character map for crunching characters. value+32 is the proper ASCII
 * representation (i.e. 63 => 63 + 32 = 95 = '_').
 */
static const char crunch_char[256] = {
	 63,  63,  63,  63,  63,  63,  63,  63,
	 63,  63,  63,  63,  63,  63,  63,  63,
	 63,  63,  63,  63,  63,  63,  63,  63,
	 63,  63,  63,  63,  63,  63,  63,  63,
	  0,   1,   2,   3,   4,   5,   6,   7,
	  8,   9,  10,  11,  12,  13,  14,  15,
	 16,  17,  18,  19,  20,  21,  22,  23,
	 24,  25,  26,  27,  28,  29,  30,  31,
	 32,  33,  34,  35,  36,  37,  38,  39,
	 40,  41,  42,  43,  44,  45,  46,  47,
	 48,  49,  50,  51,  52,  53,  54,  55,
	 56,  57,  58,  59,  60,  61,  62,  63,
	 63,  33,  34,  35,  36,  37,  38,  39,
	 40,  41,  42,  43,  44,  45,  46,  47,
	 48,  49,  50,  51,  52,  53,  54,  55,
	 56,  57,  58,  59,  60,  61,  62,  63,
	 63,  63,  63,  63,  63,  63,  63,  63,
	 63,  63,  63,  63,  63,  63,  63,  63,
	 63,  63,  63,  63,  63,  63,  63,  63,
	 63,  63,  63,  63,  63,  63,  63,  63,
	 63,  63,  63,  63,  63,  63,  63,  63,
	 63,  63,  63,  63,  63,  63,  63,  63,
	 63,  63,  63,  63,  63,  63,  63,  63,
	 63,  63,  63,  63,  63,  63,  63,  63,
	 63,  63,  63,  63,  63,  63,  63,  63,
	 63,  63,  63,  63,  63,  63,  63,  63,
	 63,  63,  63,  63,  63,  63,  63,  63,
	 63,  63,  63,  63,  63,  63,  63,  63,
	 63,  63,  63,  63,  63,  63,  63,  63,
	 63,  63,  63,  63,  63,  63,  63,  63,
	 63,  63,  63,  63,  63,  63,  63,  63,
	 63,  63,  63,  63,  63,  63,  63,  63
};

/*
 * Transform a given string to uppercase letters, transposed 32 positions
 * downward in the ASCII character map. Characters are then masked to six
 * bits, i.e. we can only express characters 32 through 95 (' ' through '_').
 * Unexpressable characters are replaced by underscores.
 */
char *duplicate_and_crunch(const char *in)
{
	const unsigned char *c;
	char *o, *out;
	const size_t size = strlen(in) + 1;

	out = malloc(size);
	if (!out)
		return NULL;

	for (c = (const unsigned char*)in, o = out; *c; c++, o++)
		*o = crunch_char[*c];

	out[size - 1] = '\0';
	return out;
}

char crunch(const char in)
{
	return crunch_char[(const unsigned char)in];
}

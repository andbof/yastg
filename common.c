#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "common.h"

const char* greek[GREEK_N] = {
	"Alpha", "Beta", "Gamma", "Delta", "Epsilon", "Zeta", "Eta", "Theta", "Iota",
	"Kappa", "Lambda", "Mu", "Nu", "Xi", "Omicron", "Pi", "Rho", "Sigma", "Tau",
	"Upsilon", "Phi", "Chi", "Psi", "Omega"
};

const char* roman[ROMAN_N] = {
	"I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI", "XII",
	"XIII", "XIV", "XV", "XVI", "XVII", "XVIII", "XIX", "XX", "XXI", "XXII",
	"XXIII", "XXIV"
};

/* upper case letter to corresponding lower case
 * letter, all invalid letters underscores */
static const char capital_to_lower[256] = {
	 95,  95,  95,  95,  95,  95,  95,  95,
	 95,  95,  95,  95,  95,  95,  95,  95,
	 95,  95,  95,  95,  95,  95,  95,  95,
	 95,  95,  95,  95,  95,  95,  95,  95,
	 32,  33,  34,  35,  36,  37,  38,  39,
	 40,  41,  42,  43,  44,  45,  46,  47,
	 48,  49,  50,  51,  52,  53,  54,  55,
	 56,  57,  58,  59,  60,  61,  62,  63,
	 64,  97,  98,  99, 100, 101, 102, 103,
	104, 105, 106, 107, 108, 109, 110, 111,
	112, 113, 114, 115, 116, 117, 118, 119,
	120, 121, 122,  91,  92,  93,  94,  95,
	 96,  97,  98,  99, 100, 101, 102, 103,
	104, 105, 106, 107, 108, 109, 110, 111,
	112, 113, 114, 115, 116, 117, 118, 119,
	120, 121, 122, 123, 124, 125, 126,  95,
	 95,  95,  95,  95,  95,  95,  95,  95,
	 95,  95,  95,  95,  95,  95,  95,  95,
	 95,  95,  95,  95,  95,  95,  95,  95,
	 95,  95,  95,  95,  95,  95,  95,  95,
	 95,  95,  95,  95,  95,  95,  95,  95,
	 95,  95,  95,  95,  95,  95,  95,  95,
	 95,  95,  95,  95,  95,  95,  95,  95,
	 95,  95,  95,  95,  95,  95,  95,  95,
	 95,  95,  95,  95,  95,  95,  95,  95,
	 95,  95,  95,  95,  95,  95,  95,  95,
	 95,  95,  95,  95,  95,  95,  95,  95,
	 95,  95,  95,  95,  95,  95,  95,  95,
	 95,  95,  95,  95,  95,  95,  95,  95,
	 95,  95,  95,  95,  95,  95,  95,  95,
	 95,  95,  95,  95,  95,  95,  95,  95,
	 95,  95,  95,  95,  95,  95,  95,  95
};

/*
 * Transform a given string to just lowercase letters,
 * with invalid letters replaced by underscores
 */
void downcase_valid(char *c)
{
	unsigned char *s = (unsigned char*)c;
	unsigned int i;
	for (i = 0; s[i] != '\0'; i++)
		s[i] = capital_to_lower[s[i]];
}

/*
 * Removes trailing newlines from a string, if any exists.
 */
void chomp(char* s)
{
	for (unsigned int len = strlen(s);
			len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r');
			s[len - 1] = '\0', len--);
}

int limit_long_to_int(const long l)
{
	if (l < INT_MIN)
		return INT_MIN;
	else if (l > INT_MAX)
		return INT_MAX;
	else
		return l;
}

unsigned int limit_long_to_uint(const long l)
{
	if (l < 0)
		return 0;
	else if (l > UINT_MAX)
		return UINT_MAX;
	else
		return l;
}

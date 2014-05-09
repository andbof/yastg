#include <errno.h>
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

int str_to_long(const char * const str, long *out)
{
	long l;
	char *end;

	errno = 0;
	l = strtol(str, &end, 10);

	if (errno)
		return -1;
	if (end == str)
		return -1;
	if (*end != '\0')
		return -1;

	*out = l;

	return 0;
}

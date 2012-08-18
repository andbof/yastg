#include <stdlib.h>
#include <string.h>
#include "defines.h"

const char* greek[GREEK_N] = {
	"Alpha", "Beta", "Gamma", "Delta", "Epsilon", "Zeta", "Eta", "Theta", "Iota", "Kappa", "Lambda",
	"Mu", "Nu", "Xi", "Omicron", "Pi", "Rho", "Sigma", "Tau", "Upsilon", "Phi", "Chi", "Psi", "Omega"
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

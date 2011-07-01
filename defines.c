#include <stdlib.h>
#include "defines.h"

const char* greek[GREEK_N] = {
  "Alpha", "Beta", "Gamma", "Delta", "Epsilon", "Zeta", "Eta", "Theta", "Iota", "Kappa", "Lambda",
  "Mu", "Nu", "Xi", "Omicron", "Pi", "Rho", "Sigma", "Tau", "Upsilon", "Phi", "Chi", "Psi", "Omega"
};

void ptr_free(void *ptr) {
  free((void*)*(int*)ptr);
}

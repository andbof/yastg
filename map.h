#ifndef _HAS_MAP_H
#define _HAS_MAP_H

#include <stdint.h>
#include "system.h"

char* generate_map(char *out, size_t len, struct system *origin,
		unsigned long radius, unsigned int width);

#endif

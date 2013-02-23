#ifndef _HAS_MAP_H
#define _HAS_MAP_H

#include <stdint.h>
#include "system.h"

char* generate_map(char * const out, const size_t len, struct system *origin,
		const unsigned long radius, const unsigned int width);

#endif

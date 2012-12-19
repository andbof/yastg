#include "data.h"
#include "planet.h"

/* Everything in this file is to be moved into configuration
 * files in the future. */

/* Planetary classification is inspired by Star Trek canon, see
 * https://en.wikipedia.org/wiki/Class_M_planet
 * http://www.sttff.net/planetaryclass.html */

struct base_type base_types[BASE_TYPE_N] = {
	{
		.name    = "Mining colony",
		.desc    = "Long description",
		.zones   = {0, 1, 0, 0},
	},
};
/*
 * Other base types:
 * Settlement, Oceanic city, City, Religious reprieve(?), Military installation,
 * Trading post, Shipyard, 
 */

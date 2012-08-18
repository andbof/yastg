#ifndef HAS_DEFINES_H
#define HAS_DEFINES_H

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/* Global compiler directives */

#define _GNU_SOURCE

/* Global struct definitions */

struct ulong_ptr {
	unsigned long i;
	void *ptr;
};

struct double_ptr {
	double i;
	void *ptr;
};

struct ptr_num {
	void* ptr;
	size_t num;
};

enum msg {
	MSG_PAUSE	= 1,
	MSG_CONT	= 2,
	MSG_WALL	= 4,
	MSG_TERM	= 8,
	MSG_RM		= 16
};

#define ERR_SYNTAX "Syntax error, see help"

/* Global short helper functions */

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#define GET_ID(x) *((size_t*)(x))
#define GET_ULONG(x) *((unsigned long*)(x))
/* #define GET_DLONG(x) *((signed long*)(x)) */
#define GET_DOUBLE(x) *((double*)(x))
#define QUOTE_(x) #x
#define QUOTE(x) QUOTE_(x)
#define __VER__ 0.1

/* Greek alphabet */

#define GREEK_N 24
#define GREEK_LEN 7
extern const char* greek[GREEK_N];

/* Roman numerals */

#define ROMAN_N 24
#define ROMAN_LEN 5
extern const char* roman[ROMAN_N];

/* Astronomical constants */

#define GM_PER_AU 150			/* gigameters per astronomical unit */
#define AU_PER_LY 63241			/* Astronomical units per light year */
#define GM_PER_LY (GM_PER_AU * AU_PER_LY)
/* The "real" values here should be habitable zone from 0.95 to 1.37 AU and snow line (i.e. the upper limit
 * for rocky planetary formation) at 2.7 AU in a sol like system.
 * For simplicity, we assume habend == snowline and change the values somewhat. */
#define HAB_ZONE_START 0.95		/* Start of habitable zone in a sol like system, in AU */
#define HAB_ZONE_END 1.50		/* End of habitable zone in a sol like system, in AU */

/* Global longer helper functions */

#define die(FMT, ...)						\
	do {							\
		printf("yastg: panic in %s:%d: " FMT "\n", __FILE__, __LINE__, __VA_ARGS__);		\
		log_printfn("panic", "in %s:%d: " FMT "\n", __FILE__, __LINE__, __VA_ARGS__);	\
		exit(EXIT_FAILURE);				\
	} while(0)

#define bug(FMT, ...)						\
	do {							\
		printf("yastg: Oops! YASTG has encountered an internal bug in %s:%d and is crashing: " FMT "\n", __FILE__, __LINE__, __VA_ARGS__);		\
		log_printfn("panic", "Oops! YASTG has encountered an internal bug in %s:%d and is crashing: " FMT "\n", __FILE__, __LINE__, __VA_ARGS__);	\
		exit(EXIT_FAILURE);				\
	} while(0)

#define MALLOC_DIE(VAR, SIZE)					\
	do {							\
		if (!(VAR = malloc(SIZE)))			\
			die("malloc %zu bytes failed", SIZE);	\
	} while(0)

#define REALLOC_DIE(VAR, SIZE)					\
	do {							\
		if (!(VAR = realloc(VAR, SIZE)))		\
			die("realloc %zu bytes failed", SIZE);	\
	} while(0)

#define MEMMOVE_DIE(DEST, ORIG, SIZE)				\
	do {							\
		if (!(memmove(DEST, ORIG, SIZE)))		\
			die("memmove %zu bytes from %p to %p failed", SIZE, ORIG, DEST);	\
	} while(0)

#define MEMCPY_DIE(DEST, ORIG, SIZE)				\
	do {							\
		if (!(memcpy(DEST, ORIG, SIZE)))		\
			die("memcpy %zu bytes from %p to %p failed", SIZE, ORIG, DEST);	\
	} while(0)

#define XYTORAD(x, y)			\
	(unsigned long)sqrt((double)x*x+y*y)

#define XYTOPHI(x, y)			\
	(atan2((double)y, (double)x))

#define POLTOX(phi, rad)		\
	(unsigned long)(rad*cos(phi))

#define POLTOY(phi, rad)		\
	(unsigned long)(rad*sin(phi))

#define MAX(x, y)			\
	((x > y) ? x : y)

#define MIN(x, y)			\
	((x < y) ? x : y)

#define mprintf(...)			\
	do {				\
		flockfile(stdout);	\
		printf(__VA_ARGS__);	\
		funlockfile(stdout);	\
	} while(0)

/* Misc global variables */

extern const char capital_to_lower[256];

/* Misc functions */

static inline void downcase_valid(char *c)
{
	unsigned char *s = (unsigned char*)c;
	unsigned int i;
	for (i = 0; s[i] != '\0'; i++)
		s[i] = capital_to_lower[s[i]];
}

/*
 * Removes trailing newlines from a string, if any exists.
 */
static inline void chomp(char* s)
{
	for (unsigned int len = strlen(s);
			len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r');
			s[len - 1] = '\0', len--);
}

#endif

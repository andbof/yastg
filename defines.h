#ifndef HAS_DEFINES_H
#define HAS_DEFINES_H

#include <stdlib.h>

struct ulong_id {
  unsigned long i;
  size_t id;
};

struct double_id {
  double i;
  size_t id;
};

struct ptr_num {
  void* ptr;
  size_t num;
};

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define _GNU_SOURCE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#define GET_ID(x) *((size_t*)(x))
#define GET_ULONG(x) *((unsigned long*)(x))
/* #define GET_DLONG(x) *((signed long*)(x)) */
#define GET_DOUBLE(x) *((double*)(x))
#define QUOTE_(x) #x
#define QUOTE(x) QUOTE_(x)
#define __VER__ 0.1

// Greek alpahabet
#define GREEK_N 24
#define GREEK_LEN 7
extern const char* greek[GREEK_N];

// Astronomical constants
#define GM_PER_AU 1496			// gigameters per astronomical unit
#define AU_PER_LY 63239			// Astronomical units per light year
#define HAB_ZONE_START 0.95		// Start of habitable zone in a sol like system, in AU
#define HAB_ZONE_END 1.37		// End of habitable zone in a sol like system, in AU
#define SNOW_LINE (2.7*GM_PER_AU)	// Upper limit of rocky planetary formation in a sol-like system
#define die(FMT, ...)				\
  do {						\
    printf("in %s:%d: " FMT "\n", __FILE__, __LINE__, __VA_ARGS__);	\
    exit(1);					\
  } while(0);

#define MALLOC_DIE(VAR, SIZE)	\
  if (!(VAR = malloc(SIZE))) die("malloc %zu bytes failed", SIZE);

#define REALLOC_DIE(VAR, SIZE)	\
  if (!(VAR = realloc(VAR, SIZE))) die("realloc %zu bytes failed", SIZE);

#define MEMMOVE_DIE(DEST, ORIG, SIZE)	\
  if (!(memmove(DEST, ORIG, SIZE))) 	\
    die("memmove %zu bytes from %p to %p failed", SIZE, ORIG, DEST);

#define MEMCPY_DIE(DEST, ORIG, SIZE)	\
  if (!(memcpy(DEST, ORIG, SIZE)))	\
    die("memcpy %zu bytes from %p to %p failed", SIZE, ORIG, DEST);

#define XYTORAD(x, y)			\
  (unsigned long)sqrt((double)x*x+y*y)

#define XYTOPHI(x, y)			\
  (atan2((double)y, (double)x))

#define POLTOX(phi, rad)		\
  (unsigned long)(rad*cos(phi))

#define POLTOY(phi, rad)		\
  (unsigned long)(rad*sin(phi))

#endif

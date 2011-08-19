#ifndef HAS_DEFINES_H
#define HAS_DEFINES_H

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

/* Global messages */

#define MSG_PAUSE	(int)1
#define MSG_CONT	(int)2
#define MSG_WALL	(int)4
#define MSG_TERM	(int)8
#define MSG_RM		(int)16

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

/* Greek alpahabet */

#define GREEK_N 24
#define GREEK_LEN 7
extern const char* greek[GREEK_N];

/* Astronomical constants */

#define GM_PER_AU 1496			// gigameters per astronomical unit
#define AU_PER_LY 63239			// Astronomical units per light year
#define HAB_ZONE_START 0.95		// Start of habitable zone in a sol like system, in AU
#define HAB_ZONE_END 1.37		// End of habitable zone in a sol like system, in AU
#define SNOW_LINE (2.7*GM_PER_AU)	// Upper limit of rocky planetary formation in a sol-like system

/* Global longer helper functions */

#define die(FMT, ...)				\
  do {						\
    log_printfn("panic", "in %s:%d: " FMT "\n", __FILE__, __LINE__, __VA_ARGS__);	\
    exit(EXIT_FAILURE);				\
  } while(0)

#define bug(FMT, ...)				\
  do {						\
    log_printfn("panic", "Oops! YASTG has encountered an internal bug in %s:%d and is crashing: " FMT "\n", __FILE__, __LINE__, __VA_ARGS__);	\
    exit(EXIT_FAILURE);				\
  } while(0)

#define MALLOC_DIE(VAR, SIZE)	\
  if (!(VAR = malloc(SIZE))) die("malloc %zu bytes failed", SIZE)

#define REALLOC_DIE(VAR, SIZE)	\
  if (!(VAR = realloc(VAR, SIZE))) die("realloc %zu bytes failed", SIZE)

#define MEMMOVE_DIE(DEST, ORIG, SIZE)	\
  if (!(memmove(DEST, ORIG, SIZE))) 	\
    die("memmove %zu bytes from %p to %p failed", SIZE, ORIG, DEST)

#define MEMCPY_DIE(DEST, ORIG, SIZE)	\
  if (!(memcpy(DEST, ORIG, SIZE)))	\
    die("memcpy %zu bytes from %p to %p failed", SIZE, ORIG, DEST)

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
  do {					\
    flockfile(stdout);			\
    printf(__VA_ARGS__);                \
    funlockfile(stdout);		\
  } while(0)

/* Misc functions */

void ptr_free(void *ptr);

#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <math.h>
#include <pthread.h>

#include "defines.h"
#include "mtrandom.h"
#include "sarray.h"
#include "array.h"
#include "test.h"
#include "server.h"
#include "sector.h"
#include "planet.h"
#include "base.h"
#include "inventory.h"
#include "player.h"
#include "universe.h"
#include "serverthread.h"
#include "parseconfig.h"
#include "civ.h"
#include "id.h"
#include "star.h"

#define PORT "2049"
#define BACKLOG 16

const char* options = "d";
int detached = 0;

void parseopts(int argc, char **argv) {
  char c;
  while ((c = getopt(argc, argv, options)) > 0) {
    switch (c) {
      case 'd': printf("Detached mode\n"); detached = 1; break;
      default:  exit(1);
    }
  }
}

/*
 * Removes a trailing newline from a string, if it exists.
 */
void chomp(char* s) {
  size_t predlen = strlen(s) -1;
  if (s[predlen] == '\n')
    s[predlen] = '\0';
}

int main(int argc, char **argv) {
  int i;
  int yes = 1;
  char *line = malloc(256); // FIXME
  struct universe *u;
  struct configtree *ctree;
  struct civ *cv;
  struct sector *s, *t;
  struct planet *pl;
  struct player *alfred;
  struct star *sol;
  size_t st;
  struct sarray *gurka;
  unsigned int tomat;
  struct sarray *civs;
  pthread_t srvthread;

  // Initialize
  srand(time(NULL));
  mtrandom_init();
  init_id();

#ifdef TEST
  run_tests();
  exit(0);
#endif

  // Parse command line options
  parseopts(argc, argv);

  // Load config files
  printf("Parsing configuration files\n");
  printf("  civilizations: ");
  civs = loadcivs();
  printf("done, %zu civs loaded.\n", civs->elements);

  // Create universe
  printf("Creating universe\n");
  u = createuniverse(civs);

  // Create player
  printf("Creating player\n");
  alfred = malloc(sizeof(struct player));
  alfred->name = "Alfred";
  alfred->position = GET_ID(u->sectors->array);

  // Initialize screen (this fixes the screen/console mutex)
  pthread_mutex_init(&stdout_mutex, NULL);

  // Now GO!
  mprintf("Starting server ...\n");
  if ((i = pthread_create(&srvthread, NULL, server_main, NULL)))
    die("Could not launch server thread, error %d\n", i);

  mprintf("Welcome to YASTG v%s (commit %s), built %s %s.\n\n", QUOTE(__VER__), QUOTE(__COMMIT__), __DATE__, __TIME__);

  mprintf("Universe has %zu sectors in total\n", u->sectors->elements);
  while (1) {
    mprintf("console> ");
    fgets(line, 256, stdin);
    chomp(line);
    if (!strcmp(line,"help")) {
      mprintf("No help available.\n");
    } else if (!strcmp(line, "quit")) {
      mprintf("Bye!\n");
      exit(0);
    }
  }

  while (1) {
    s = sarray_getbyid(u->sectors, &alfred->position);
    printf("You are in sector %s (id %zx, coordinates %ldx%ld), habitability %d\n", s->name, s->id, s->x, s->y, s->hab);
    printf("Snow line at %lu Gm\n", s->snowline);
    printf("Habitable zone is from %lu to %lu Gm\n", s->hablow, s->habhigh);
    for (i = 0; i < s->stars->elements; i++) {
      sol = (struct star*)sarray_getbypos(s->stars, i);
      printf("%s: Class %c %s (id %zx)\n", sol->name, stellar_cls[sol->cls], stellar_lum[sol->lum], sol->id);
      printf("  Surface temperature: %dK, habitability modifier: %d, luminosity: %s \n", sol->temp, sol->hab, hundreths(sol->lumval));
    }
    printf("This sector has hyperspace links to\n");
    for (i = 0; i < s->linkids->elements; i++) {
      printf("  %s\n", ((struct sector*)sarray_getbyid(u->sectors, &GET_ID(sarray_getbypos(s->linkids, i))))->name);
    }
    printf("Sectors within 50 lys are:\n");
    gurka = getneighbours(u, s, 50);
    for (i = 0; i < gurka->elements; i++) {
      t = (struct sector*)sarray_getbyid(u->sectors, sarray_getbypos(gurka, i));
      printf("  %s at %lu ly\n", t->name, getdistance(s, t));
    }
    if (s->owner != 0) {
      printf("This sector is owned by civ %zx\n", s->id);
    } else {
      printf("This sector is not part of any civilization\n");
    }

    printf("> ");
    fgets(line, 256, stdin);
    chomp(line);
    if (!strcmp(line,"help")) {
      printf("No help available.\n");
    } else if (!strncmp(line, "go ", 3)) {
      alfred->position = GET_ID((struct sector*)sarray_getbyname(u->sectors, line+3));
      printf("Entering %s\n", ((struct sector*)sarray_getbyid(u->sectors, &alfred->position))->name);
    } else if (!strcmp(line, "quit")) {
      printf("Bye!\n");
      exit(0);
    }
  }

  /* Destroy all structures and free all memory */

  sarray_free(civs);
  free(civs);
  printf("Done!\n");
  free(line);

  exit(0);

  return 0;

}


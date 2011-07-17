#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "defines.h"
#include "log.h"
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
#include "parseconfig.h"
#include "civ.h"
#include "id.h"
#include "star.h"

#define PORT "2049"
#define BACKLOG 16

const char* options = "d";
int detached = 0;

extern int sockfd;

static pthread_t srvthread;
static int srvfd[2];

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
  int running = 1;
  char *line = malloc(256); // FIXME
  struct configtree *ctree;
  struct civ *cv;
  struct sector *s, *t;
  struct planet *pl;
  struct star *sol;
  size_t st;
  struct sarray *gurka;
  unsigned int tomat;
  struct sarray *civs;

  // Open log file
  log_init();
  log_printfn("main", "YASTG initializing");
  log_printfn("main", "v%s (commit %s), built %s %s", QUOTE(__VER__), QUOTE(__COMMIT__), __DATE__, __TIME__);

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
  univ = createuniverse(civs);

  // Start server thread
  if (pipe(srvfd) != 0)
    die("%s", "Could not create server pipe");
  if (pthread_create(&srvthread, NULL, server_main, &srvfd[0]))
    die("%s", "Could not launch server thread");

  mprintf("Welcome to YASTG v%s (commit %s), built %s %s.\n\n", QUOTE(__VER__), QUOTE(__COMMIT__), __DATE__, __TIME__);

  mprintf("Universe has %zu sectors in total\n", univ->sectors->elements);
  while (running) {
    mprintf("console> ");
    fgets(line, 256, stdin); // FIXME
    chomp(line);
    if (!strcmp(line,"help")) {
      mprintf("No help available.\n");
    } else if (!strcmp(line, "quit")) {
      mprintf("Bye!\n");
      running = 0;
    }
  }

  /*
  while (1) {
    s = sarray_getbyid(univ->sectors, &alfred->position);
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
      printf("  %s\n", ((struct sector*)sarray_getbyid(univ->sectors, &GET_ID(sarray_getbypos(s->linkids, i))))->name);
    }
    printf("Sectors within 50 lys are:\n");
    gurka = getneighbours(univ, s, 50);
    for (i = 0; i < gurka->elements; i++) {
      t = (struct sector*)sarray_getbyid(univ->sectors, sarray_getbypos(gurka, i));
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
      alfred->position = GET_ID((struct sector*)sarray_getbyname(univ->sectors, line+3));
      printf("Entering %s\n", ((struct sector*)sarray_getbyid(univ->sectors, &alfred->position))->name);
    } else if (!strcmp(line, "quit")) {
      printf("Bye!\n");
      exit(0);
    }
  }
  */

  /* Kill server thread, this will also kill all player threads */
  i = MSG_TERM;
  st = 0;
  if (write(srvfd[1], &i, sizeof(i)) < 1)
    bug("%s", "server signalling fd seems closed when sending signal");
  if (write(srvfd[1], &st, sizeof(st)) < 1)
    bug("%s", "server signalling fd seems closed when sending parameter");
  log_printfn("main", "waiting for server to terminate");
  pthread_join(srvthread, NULL);

  /* Destroy all structures and free all memory */

  log_printfn("main", "freeing all structs");
  sarray_free(civs);
  free(civs);
  free(line);
  universe_free(univ);
  id_destroy();

  log_close();

  return 0;

}


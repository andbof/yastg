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
#include <malloc.h>

#include "defines.h"
#include "log.h"
#include "mtrandom.h"
#include "sarray.h"
#include "array.h"
#include "ptrarray.h"
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
  size_t st, su;
  struct sarray *gurka;
  unsigned int tomat;
  struct array *civs;
  struct mallinfo minfo;

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
  univ = universe_create();
  universe_init(civs);

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
    } else if (!strcmp(line, "memstat")) {
      minfo = mallinfo();
      mprintf("Memory statistics:\n");
      mprintf("  Memory allocated with sbrk by malloc:           %d bytes\n", minfo.arena);
      mprintf("  Number of chunks not in use:                    %d\n", minfo.ordblks);
      mprintf("  Number of chunks allocated with mmap:           %d\n", minfo.hblks);
      mprintf("  Memory allocated with mmap:                     %d bytes\n", minfo.hblkhd);
      mprintf("  Memory occupied by chunks handed out by malloc: %d bytes\n", minfo.uordblks);
      mprintf("  Memory occupied by free chunks:                 %d bytes\n", minfo.fordblks);
      mprintf("  Size of top-most releasable chunk:              %d bytes\n", minfo.keepcost);
    } else if (!strcmp(line, "stats")) {
      mprintf("Statistics:\n");
      mprintf("  Size of universe:          %zu sectors\n", univ->sectors->elements);
      mprintf("  Number of users known:     %s\n", "FIXME");
      mprintf("  Number of users connected: %s\n", "FIXME");
    } else if (!strcmp(line, "quit")) {
      mprintf("Bye!\n");
      running = 0;
    } else {
      mprintf("Unknown command or syntax error.\n");
    }
  }

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

  log_printfn("main", "cleaning up");
  for (st = 0; st < univ->sectors->elements; st++) {
    s = ptrarray_get(univ->sectors, st);
    sector_free(s);
  }
  array_free(civs);
  free(civs);
  free(line);
  universe_free(univ);
  id_destroy();

  log_close();

  return 0;

}


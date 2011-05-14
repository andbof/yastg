#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <math.h>

#include "defines.h"
#include "sarray.h"
#include "array.h"
#include "test.h"
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

void preparesocket(struct addrinfo **servinfo) {
  int i;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  if ((i = getaddrinfo(NULL, PORT, &hints, servinfo)) != 0) {
    die("getaddrinfo: %s\n", gai_strerror(i));
  }
}

void sigchild_handler(int s) {
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

/*
 * Removes a trailing newline from a string, if it exists.
 */
void chomp(char* s) {
  if (s[strlen(s)-1] == '\n') {
    s[strlen(s)-1] = '\0';
  }
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char **argv) {
  struct addrinfo *servinfo, *p;
  int i;
  socklen_t sin_size;
  struct sockaddr_storage peer_addr;
  char peer[INET6_ADDRSTRLEN];
  int sockfd, newfd;
  int yes = 1;
  struct sigaction sa;
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

  // Initialize
  srand(time(NULL));
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
  u = createuniverse(civs);

  // Create player
  printf("Creating player\n");
  alfred = malloc(sizeof(struct player));
  alfred->name = "Alfred";
  alfred->position = GET_ID(u->sectors->array);

  // Now GO!
  printf("Welcome to YASTG v%s (commit %s), built %s %s.\n\n", QUOTE(__VER__), QUOTE(__COMMIT__), __DATE__, __TIME__);

  printf("Universe has %zu sectors in total\n", u->sectors->elements);
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

  array_free(civs);
  free(civs);
  printf("Done!\n");
  free(line);

  exit(0);

  preparesocket(&servinfo);
  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      printf("%s", "socket error");
      continue;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == 1) {
      die("%s", "setsockopt error");
    }
    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      printf("%s", "bind error");
      continue;
    }
    break;
  }
  if (p == NULL) {
    die("%s", "server failed to bind");
  }
  freeaddrinfo(servinfo); // No longer needed
  if (listen(sockfd, BACKLOG) == -1) {
    die("%s", "listen error");
  }
  sa.sa_handler = sigchild_handler;	// Reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags =	SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    die("%s", "sigaction error");
  }
  printf("Server is waiting for connections\n");

  while (1) {
    sin_size = sizeof(peer_addr);
    newfd = accept(sockfd, (struct sockaddr*)&peer_addr, &sin_size);
    if (newfd == -1) {
      die("%s", "socket accept error");
    }
    inet_ntop(peer_addr.ss_family, get_in_addr((struct sockaddr*)&peer_addr), peer, sizeof(peer));
    printf("Client %s connected\n", peer);
    if (!fork()) {
      // Child process
      close(sockfd);	// Not needed in child
      if (send(newfd, "Hello world!", 13, 0) == -1) {
	die("%s", "sending error");
      }
      close(newfd);
      exit(0);
    } else {
      // Parent process
      close(newfd);	// Not needed in parent
    }
  }

  return 0;

}

/*
struct addrinfo hints;
struct addrinfo *servinfo;
int status;

memset(&hints, 0, sizeof(hints));
hints.ai_family = AF_UNSPEC;		// Don't care if IPv4 or IPv6

int main() {

  if (!(status = getaddrinfo(NULL, "2049", &hints, &servinfo))) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(1);
  }

  return 0;
}

*/

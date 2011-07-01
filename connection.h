#ifndef HAS_SERVERTHREAD_H
#define HAS_SERVERTHREAD_H

#include <pthread.h>
#include <arpa/inet.h>
#include "player.h"

#define CONN_BUFSIZE 1500
#define CONN_MAXBUFSIZE 10240

struct conndata {
  unsigned long id;
  pthread_t thread;
  int fd;
  pthread_mutex_t fd_mutex;
  char peer[INET6_ADDRSTRLEN];
  struct player *pl;
  size_t rbufs, sbufs;
  char *rbuf;
  char *sbuf;
};

int conn_init(struct conndata *data);
void conn_cleanexit(struct conndata *data);
void conn_send(struct conndata *data, char *format, ...);
void* conn_main(void *dataptr);

#endif

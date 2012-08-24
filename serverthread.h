#ifndef HAS_SERVERTHREAD_H
#define HAS_SERVERTHREAD_H

#include <pthread.h>
#include <arpa/inet.h>

struct serverthreaddata {
  pthread_t thread;
  int fd;
  char peer[INET6_ADDRSTRLEN];
};

void* serverthread_main(void *dataptr);

#endif

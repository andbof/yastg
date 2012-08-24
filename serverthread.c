#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>

#include "serverthread.h"
#include "defines.h"

void serverthread_cleanup(struct serverthreaddata *data) {
  close(data->fd);
  // FIXME: Close mutexes & deallocate everything
}

void serverthread_loop(struct serverthreaddata *data) {
  int running = 1;
  while (running) {
    if (send(data->fd, "Hello world!\n", 13, 0) == -1)
      running = 0;
  }
}

void* serverthread_main(void *dataptr) {
  struct serverthreaddata *data = dataptr;
  serverthread_loop(data);
  serverthread_cleanup(data);
  return 0;
}

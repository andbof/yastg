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

#include "defines.h"
#include "serverthread.h"

void serverthread_cleanup(struct serverthreaddata *data) {
  close(data->fd);
  // FIXME: Close mutexes & deallocate everything
}

void serverthread_loop(struct serverthreaddata *data) {
  while (1) {
    if (send(data->fd, "Hello world!\n", 13, MSG_NOSIGNAL) == -1)
      break;
  }
}

void* serverthread_main(void *dataptr) {
  struct serverthreaddata *data = dataptr;
  mprintf("Thread for peer %s started\n", data->peer);
  serverthread_loop(data);
  mprintf("Thread for peer %s closing down\n", data->peer);
  serverthread_cleanup(data);
  mprintf("Thread for peer %s finished\n", data->peer);
  return 0;
}

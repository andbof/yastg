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
#include "log.h"
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
  log_printfn("serverthread", "successfully created thread for peer %s", data->peer);
  serverthread_loop(data);
  log_printfn("serverthread", "peer %s disconnected, cleaning up", data->peer);
  serverthread_cleanup(data);
  log_printfn("serverthread", "cleanup complete for peer %s", data->peer);
  return 0;
}

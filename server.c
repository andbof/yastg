#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
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
#include "server.h"
#include "connection.h"
#include "id.h"

#define PORT "2049"
#define BACKLOG 16	// Size of pending connections queue

static int tnum = 0;
static struct conndata std[10];
static pthread_t srvthread;
static fd_set fdset;
static int sockfd, signfd, maxfd;

void server_cleanexit() {
  int i, *j;
  int msg = MSG_TERM;
  log_printfn("server", "sending terminate to all player threads");
  for (i = 0; i < tnum; i++) {
    if (write(std[i].threadfds[1], &msg, sizeof(msg)) < 1)
      bug("thread %lx's signalling fd seems closed", std[i].id);
  }
  log_printfn("server", "waiting for all player threads to terminate");
  for (i = 0; i < tnum; i++) {
    pthread_join(std[i].thread, NULL);
  }
  log_printfn("server", "server shutting down");
  close(sockfd);
  pthread_exit(0);
}

void server_handlesignal(int signal) {
  switch (signal) {
    case MSG_TERM:
      server_cleanexit();
      break;
    default:
      log_printfn("server", "unknown message received: %d", signal);
  }
}

void server_preparesocket(struct addrinfo **servinfo) {
  int i;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;		// Don't care if we bind to IPv4 or IPv6 sockets
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  if ((i = getaddrinfo(NULL, PORT, &hints, servinfo)) != 0) {
    die("getaddrinfo: %s\n", gai_strerror(i));
  }
}

// get sockaddr, IPv4 or IPv6:
void* server_get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  } else {
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
  }
}

int server_setupsocket() {
  struct addrinfo *servinfo, *p;
  server_preparesocket(&servinfo);
  int fd;
  int yes = 1;

  // We loop trough each ai structure to get the first one that actually works
  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      mprintf("%s", "socket error");
      continue;
    }
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == 1)
      die("%s", "setsockopt error");
    if (bind(fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(fd);
      mprintf("%s", "bind error");
      continue;
    }
    break;
  }
  if (p == NULL)
    die("%s", "server failed to bind"); // None of them worked
  if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
    die("failed setting server socket to non-blocking, error %s", strerror(errno));
  if (listen(fd, BACKLOG) == -1)
    die("%s", "listen error");

  // The servinfo structure was only used to get an ai struct, so we can free it now
  freeaddrinfo(servinfo);

  return fd;
}

char* getpeer(struct sockaddr_storage sock) {
  struct sockaddr_in *s4 = (struct sockaddr_in*)&sock;
  struct sockaddr_in6 *s6 = (struct sockaddr_in6*)&sock;
  char *result = malloc(INET6_ADDRSTRLEN + 6);
  if (sock.ss_family == AF_INET) {
    inet_ntop(AF_INET, &s4->sin_addr, result, INET6_ADDRSTRLEN + 6);
    sprintf(result + strlen(result), ":%d", ntohs(s4->sin_port));
  } else {
    inet_ntop(AF_INET6, &s6->sin6_addr, result, INET6_ADDRSTRLEN + 6);
    sprintf(result + strlen(result), ":%d", ntohs(s6->sin6_port));
  }
  return result;
}

void* server_main(void* p) {
  int i, j;
  socklen_t sin_size;
  size_t st;
  struct sockaddr_storage peer_addr;

  srvthread = pthread_self();
  signfd = *(int*)p;
  sockfd = server_setupsocket();
  maxfd = MAX(sockfd, signfd);
  maxfd++;

  log_printfn("server", "server is up waiting for connections on port %s", PORT);

  while (1) {
    FD_ZERO(&fdset);
    FD_SET(sockfd, &fdset);
    FD_SET(signfd, &fdset);
    i = select(maxfd, &fdset, NULL, NULL, NULL);
    if (i == -1)
      die("select() failed in server, error %s", strerror(errno));
    if (FD_ISSET(signfd, &fdset)) {
      // We have received a message on the signalling file handle
      read(signfd, &i, sizeof(i));
      server_handlesignal(i);
    } else {
      // We have received a new connection, accept it
      sin_size = sizeof(peer_addr);
      std[tnum].peerfd = accept(sockfd, (struct sockaddr*)&peer_addr, &sin_size);
      if (std[tnum].peerfd == -1) {
	if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
	  log_printfn("server", "a peer disconnected before connection could be properly accepted");
	} else {
	  log_printfn("server", "could not accept socket connection, error %s", strerror(errno));
	}
      } else {
//	inet_ntop(peer_addr.ss_family, server_get_in_addr((struct sockaddr*)&peer_addr), std[tnum].peer, sizeof(std[tnum].peer));
        st = sizeof(std[tnum].sock);
	getpeername(std[tnum].peerfd, (struct sockaddr*)&std[tnum].sock, &st);
	std[tnum].peer = getpeer(std[tnum].sock);
	log_printfn("server", "new connection from %s, assigning id %lx", std[tnum].peer, std[tnum].id);
	if (!conn_init(&std[tnum])) {
	  log_printfn("server", "failed initializing thread to handle connection %lx", std[tnum].id);
	  conn_clean(&std[tnum]);
	} else {
	  if ((i = pthread_create(&std[tnum].thread, NULL, conn_main, &std[tnum]))) {
	    log_printfn("failed creating thread to handle connection from %s: %i", std[tnum].peer, i);
	  }
	  tnum++;
	}
      }
    }
  }

}

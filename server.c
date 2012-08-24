#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
#include "server.h"
#include "serverthread.h"

#define PORT "2049"
#define BACKLOG 16	// Size of pending connections queue

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
  int sockfd;
  int yes = 1;

  // We loop trough each ai structure to get the first one that actually works
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
  if (p == NULL)
    die("%s", "server failed to bind"); // None of them worked
  if (listen(sockfd, BACKLOG) == -1)
    die("%s", "listen error");

  // The servinfo structure was only used to get an ai struct, so we can free it now
  freeaddrinfo(servinfo);

  return sockfd;
}

void* server_main(void* p) {
  int i;
  socklen_t sin_size;
  struct sockaddr_storage peer_addr;
  char peer[INET6_ADDRSTRLEN];
  int sockfd, newfd;
  int yes = 1;
  struct serverthreaddata std[10];
  int j;

  sockfd = server_setupsocket();

  printf("YASTG server is waiting for connections\n");

  j = 0;
  while (1) {
    sin_size = sizeof(peer_addr);
    std[j].fd = accept(sockfd, (struct sockaddr*)&peer_addr, &sin_size);
    if (std[j].fd == -1) {
      printf("Could not accept socket connection\n");
    } else {
      inet_ntop(peer_addr.ss_family, server_get_in_addr((struct sockaddr*)&peer_addr), std[j].peer, sizeof(std[j].peer));
      printf("Client %s connected\n", std[j].peer);
      if ((i = pthread_create(&std[j].thread, NULL, serverthread_main, &std[j]))) {
	printf("Could not create new thread for peer %s: %d\n", std[j].peer, i);
      } else {
	printf("New thread for peer %s successfully created\n", std[j].peer);
      }
      j++;
    }
  }

}

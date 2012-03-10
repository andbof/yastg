#ifndef HAS_CONNETION_H
#define HAS_CONNETION_H

#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "player.h"

#define CONN_BUFSIZE 1500
#define CONN_MAXBUFSIZE 10240

struct conndata {
	size_t id;
	pthread_t thread;
	fd_set rfds;
	int peerfd, serverfd, threadfds[2];
	pthread_mutex_t fd_mutex;
	struct sockaddr_storage sock;
	char *peer;
	struct player *pl;
	size_t rbufs, sbufs;
	char *rbuf;
	char *sbuf;
	int paused;
};

struct conndata* conn_create();
void conndata_free(void *ptr);
void conn_cleanexit(struct conndata *data);
void conn_send(struct conndata *data, char *format, ...);
void* conn_main(void *dataptr);

void conn_signalserver(struct conndata *data, int signal, size_t param);

#endif

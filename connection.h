#ifndef _HAS_CONNECTION_H
#define _HAS_CONNECTION_H

#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "player.h"
#include "server.h"

#define CONN_BUFSIZE 1500
#define CONN_MAXBUFSIZE 10240

struct conndata {
	uint32_t id;
	pthread_t thread;
	fd_set rfds;
	int peerfd, serverfd, threadfds[2];
	struct sockaddr_storage sock;
	char *peer;
	struct player *pl;
	size_t rbufs, sbufs;
	size_t rbufi;
	char *rbuf;
	char *sbuf;
	int paused;
	struct list_head list;
};

struct conndata* conn_create();
void conndata_free(void *ptr);
void* conn_main(void *dataptr);
void conn_send(struct conndata *data, char *format, ...);
void conn_cleanexit(struct conndata *data);
void conn_error(struct conndata *data, char *format, ...);

#endif

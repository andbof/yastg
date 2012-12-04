#ifndef _HAS_CONNECTION_H
#define _HAS_CONNECTION_H

#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ev.h>
#include "player.h"
#include "server.h"
#include "ptrlist.h"

#define CONN_BUFSIZE 1500
#define CONN_MAXBUFSIZE 10240

struct connection {
	uint32_t id;
	ev_io watcher;
	fd_set rfds;
	int peerfd, serverfd;
	struct sockaddr_storage sock;
	char peer[INET6_ADDRSTRLEN + 7];
	struct player *pl;
	size_t rbufs, sbufs;
	size_t rbufi;
	char *rbuf;
	char *sbuf;
	int paused;
	struct list_head list, work;
};

struct conn_data {
	struct list_head workers;
	pthread_mutex_t workers_lock;
	pthread_cond_t workers_cond;
	struct ptrlist work_items;
};

struct conn_worker_list {
	pthread_t thread;
	volatile int terminate;
	struct conn_data *conn_data;
	struct list_head list;
};

int conn_init(struct connection *conn);
void connection_free(struct connection *conn);
void* conn_main(void *dataptr);
void conn_send(struct connection *data, char *format, ...);
void conn_cleanexit(struct connection *data);
void conn_error(struct connection *data, char *format, ...);
void* connection_worker(void *_data);
int conn_fulfixinit(struct connection *data);

void conn_do_work(struct conn_data *data, struct connection *conn);

int conndata_init(struct conn_data *data);
void conn_shutdown(struct conn_data *data);
void conn_destroy(struct conn_data *data);

#endif

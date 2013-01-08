#ifndef _HAS_SERVER_H
#define _HAS_SERVER_H

#include <pthread.h>
#include "connection.h"

struct server {
	pthread_t thread;
	int fd[2];
	int running;
};

struct signal {
	uint16_t cnt;
	enum msg type;
} __attribute__((packed));

void server_disconnect_nicely(struct connection *conn);
void initialize_server(struct server * const server);
int start_server(struct server * const server);
void stop_server(struct server * const server);

#endif

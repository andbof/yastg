#ifndef _HAS_CONSOLE_H
#define _HAS_CONSOLE_H

#include <pthread.h>
#include "list.h"
#include "server.h"

struct console {
	struct server *server;
	struct list_head cli;
	int running;
	pthread_mutex_t running_lock;
	pthread_t thread;
};

void console_init(struct console * const console, struct server * const server);
void console_free(struct console * const console);
int start_console(struct console * const console);
void stop_console(struct console * const console);

#endif

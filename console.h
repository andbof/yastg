#ifndef _HAS_CONSOLE_H
#define _HAS_CONSOLE_H

#include <ev.h>
#include <pthread.h>
#include "list.h"
#include "server.h"

struct console {
	struct server *server;
	struct list_head cli;
	struct ev_loop *loop;
	int sleep;
	ev_async kill_watcher;
	ev_io cmd_watcher;
	pthread_t thread;
	struct buffer buffer;
	void (*print)(void*, const char*, ...);
};

void console_init(struct console * const console, struct server * const server,
		void (*print)(void*, const char*, ...));
void console_free(struct console * const console);
int start_console(struct console * const console);
void stop_console(struct console * const console);

#endif

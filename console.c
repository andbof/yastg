#include <dlfcn.h>
#include <malloc.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ev.h>
#include <config.h>
#include "console.h"
#include "common.h"
#include "buffer.h"
#include "cli.h"
#include "item.h"
#include "list.h"
#include "log.h"
#include "module.h"
#include "planet.h"
#include "planet_type.h"
#include "port.h"
#include "server.h"
#include "universe.h"

static void write_msg(int fd, struct signal *msg, char *msgdata)
{
	if (write(fd, msg, sizeof(*msg)) < 1)
		bug("%s", "server signalling fd is closed");
	if (msg->cnt && write(fd, msgdata, msg->cnt) < 1)
		bug("%s", "server signalling fd is closed");
}

static void enter_sleep(struct console * const console)
{
	console->sleep = 1;
	ev_io_stop(console->loop, &console->cmd_watcher);
}

static int cmd_ports(void *_console, char *param)
{
	struct port_type *type;

	printf("%-26s %-26s %-8s %-8s %-8s %-8s\n",
			"Name", "Description", "OCEAN", "SURFACE",
			"ORBIT", "ROGUE");
	list_for_each_entry(type, &univ.port_types, list)
		printf("%-26.26s %-26.26s %-8s %-8s %-8s %-8s\n",
				type->name, type->desc,
				(type->zones[OCEAN] ? "Yes" : "No"),
				(type->zones[SURFACE] ? "Yes" : "No"),
				(type->zones[ORBIT] ? "Yes" : "No"),
				(type->zones[ROGUE] ? "Yes" : "No"));

	return 0;
}

static int cmd_help(void *_console, char *param)
{
	struct console *console = _console;
	cli_print_help(stdout, &console->cli);

	return 0;
}

static int cmd_insmod(void *_console, char *param)
{
	int r;

	if (!param) {
		printf("usage: insmod <file name.so>\n");
		return 0;
	}

	r = module_insert(param);
	if (r != 0)
		printf("Error inserting module: %s\n",
				(r == MODULE_DL_ERROR ? dlerror() : module_strerror(r))
		       );

	return 0;
}

static int cmd_items(void *_console, char *param)
{
	struct item *i;

	printf("%-24s %-8s\n", "Name", "Weight");
	list_for_each_entry(i, &univ.items, list)
		printf("%-24.24s %-8ld\n", i->name, i->weight);

	return 0;
}

static int cmd_lsmod(void *_console, char *param)
{
	struct module *m;

	if (list_empty(&modules_loaded)) {
		printf("No modules are currently loaded\n");
		return 0;
	}

	printf("%-16s %-8s\n", "Module", "Size");
	list_for_each_entry(m, &modules_loaded, list)
		printf("%-16s %-8d\n", m->name, m->size);

	return 0;
}

static int cmd_wall(void *_console, char *message)
{
	struct console *console = _console;
	if (message) {
		struct signal msg = {
			.cnt = strlen(message) + 1,
			.type = MSG_WALL
		};
		write_msg(console->server->fd[1], &msg, message);
	} else {
		printf("usage: wall <message>\n");
	}
	return 0;
}

static int cmd_pause(void *_console, char *param)
{
	struct console *console = _console;
	struct signal msg = {
		.cnt = 0,
		.type = MSG_PAUSE
	};
	write_msg(console->server->fd[1], &msg, NULL);
	return 0;
}

static int cmd_planets(void *_console, char *param)
{
	struct planet_type *type;

	printf("%-5s %-26s %-4s %-4s %-4s %-12s %-12s %-10s\n",
			"Class", "Name", "HOT", "ECO", "COLD",
			"Min life", "Max life", "Port types");
	list_for_each_entry(type, &univ.planet_types, list)
		printf("%c     %-26.26s %-4.4s %-4.4s %-4.4s %-12.12s %-12.12s %-10lu\n",
				type->c, type->name,
				(type->zones[HOT] ? "Yes" : "No"),
				(type->zones[ECO] ? "Yes" : "No"),
				(type->zones[COLD] ? "Yes" : "No"),
				(planet_life_names[type->minlife]),
				(planet_life_names[type->maxlife]),
				ptrlist_len(&type->port_types));

	return 0;
}

static int cmd_resume(void *_console, char *param)
{
	struct console *console = _console;
	struct signal msg = {
		.cnt = 0,
		.type = MSG_CONT
	};
	write_msg(console->server->fd[1], &msg, NULL);
	return 0;
}

static void _cmd_rmmod(struct module *m)
{
	int r;

	r = module_remove(m);
	if (r)
		printf("Error removing module: %s\n",
				(r == MODULE_DL_ERROR ? dlerror() : module_strerror(r))
		       );
}

static int cmd_rmmod(void *_console, char *name)
{
	struct module *m;

	if (!name) {
		printf("usage: rmmod <module>\n");
		return 0;
	}

	list_for_each_entry(m, &modules_loaded, list) {
		if (strcmp(m->name, name) == 0) {
			_cmd_rmmod(m);
			return 0;
		}
	}

	printf("Module %s is not currently loaded\n", name);
	return 0;
}

static int cmd_ships(void *_console, char *param)
{
	struct ship_type *type;

	printf("%-26s %-26s %-12s\n",
			"Name", "Description", "Carry weight");
	list_for_each_entry(type, &univ.ship_types, list)
		printf("%-26.26s %-26.26s %-12d\n",
				type->name, type->desc, type->carry_weight);

	return 0;
}

static int cmd_memstat(void *_console, char *param)
{
	struct mallinfo minfo = mallinfo();
	printf("Memory statistics:\n"
			"  Memory allocated with sbrk by malloc:           %d bytes\n"
			"  Number of chunks not in use:                    %d\n"
			"  Number of chunks allocated with mmap:           %d\n"
			"  Memory allocated with mmap:                     %d bytes\n"
			"  Memory occupied by chunks handed out by malloc: %d bytes\n"
			"  Memory occupied by free chunks:                 %d bytes\n"
			"  Size of top-most releasable chunk:              %d bytes\n",
			minfo.arena, minfo.ordblks, minfo.hblks, minfo.hblkhd,
			minfo.uordblks, minfo.fordblks, minfo.keepcost);
	return 0;
}

static int cmd_stats(void *_console, char *param)
{
	struct tm t;
	char created[32];
	memset(created, 0, sizeof(created));

	localtime_r(&univ.created, &t);
	strftime(created, sizeof(created), "%c", &t);

	printf("Statistics:\n"
			"  Size of universe:          %lu systems\n"
			"  Universe created:          %s\n"
			"  Number of users known:     %s\n"
			"  Number of users connected: %s\n",
			ptrlist_len(&univ.systems),
			created,
			"FIXME", "FIXME");
	return 0;
}

static int cmd_quit(void *_console, char *param)
{
	struct console *console = _console;
	printf("Bye!\n");
	enter_sleep(console);
	kill(getpid(), SIGTERM);
	return 0;
}

static int register_console_commands(struct console * console)
{
	if (cli_add_cmd(&console->cli, "ports", cmd_ports, console, "List available ports"))
		goto err;
	if (cli_add_cmd(&console->cli, "help", cmd_help, console, "Display this help text"))
		goto err;
	if (cli_add_cmd(&console->cli, "insmod", cmd_insmod, console, "Insert a loadable module"))
		goto err;
	if (cli_add_cmd(&console->cli, "items", cmd_items, console, "List available items"))
		goto err;
	if (cli_add_cmd(&console->cli, "lsmod", cmd_lsmod, console, "List modules currently loaded"))
		goto err;
	if (cli_add_cmd(&console->cli, "wall", cmd_wall, console, "Send a message to all connected players"))
		goto err;
	if (cli_add_cmd(&console->cli, "pause", cmd_pause, console, "Pause all players"))
		goto err;
	if (cli_add_cmd(&console->cli, "planets", cmd_planets, console, "List available planet types"))
		goto err;
	if (cli_add_cmd(&console->cli, "resume", cmd_resume, console, "Resume all players"))
		goto err;
	if (cli_add_cmd(&console->cli, "rmmod", cmd_rmmod, console, "Unload a loadable module"))
		goto err;
	if (cli_add_cmd(&console->cli, "ships", cmd_ships, console, "List available ship types"))
		goto err;
	if (cli_add_cmd(&console->cli, "stats", cmd_stats, console, "Display statistics"))
		goto err;
	if (cli_add_cmd(&console->cli, "memstat", cmd_memstat, console, "Display memory statistics"))
		goto err;
	if (cli_add_cmd(&console->cli, "quit", cmd_quit, console, "Terminate the server"))
		goto err;

	return 0;

err:
	cli_tree_destroy(&console->cli);
	return -1;
}

#define CONSOLE_PROMPT "console> "
static void console_cmd_cb(struct ev_loop * const loop, ev_io * const w, const int revents)
{
	struct console *console = w->data;

	if (read_into_buffer(STDIN_FILENO, &console->buffer))
		return;

	if (!buffer_terminate_line(&console->buffer)) {
		if (strlen(console->buffer.buf) > 0 && cli_run_cmd(&console->cli, console->buffer.buf) < 0)
			printf("Unknown command or syntax error.\n");

		if (console->sleep)
			return;

		buffer_reset(&console->buffer);
		printf(CONSOLE_PROMPT);
		fflush(stdout);
	}
}

static void console_kill_cb(struct ev_loop * const loop, struct ev_async *w, int revents)
{
	ev_break(loop, EVBREAK_ALL);
}

static void* console_main(void *_console)
{
	struct console *console = _console;
	INIT_LIST_HEAD(&console->cli);

	console->loop = ev_loop_new(EVFLAG_AUTO | EVFLAG_NOSIGMASK);
	if (!console->loop)
		die("%s", "Could not initialize event loop");

	if (register_console_commands(console))
		die("%s", "Could not register console commands");

	ev_async_init(&console->kill_watcher, console_kill_cb);
	ev_io_init(&console->cmd_watcher, console_cmd_cb, STDIN_FILENO, EV_READ);
	console->kill_watcher.data = console;
	console->cmd_watcher.data = console;
	ev_async_start(console->loop, &console->kill_watcher);
	ev_io_start(console->loop, &console->cmd_watcher);

	printf("Welcome to YASTG %s, built %s %s.\n\n", PACKAGE_VERSION, __DATE__, __TIME__);
	printf("Universe has %lu systems in total\n", ptrlist_len(&univ.systems));
	printf("\n" CONSOLE_PROMPT);
	fflush(stdout);

	ev_run(console->loop, 0);

	ev_io_stop(console->loop, &console->cmd_watcher);
	ev_async_stop(console->loop, &console->kill_watcher);
	cli_tree_destroy(&console->cli);
	ev_loop_destroy(console->loop);

	return 0;
}

void console_init(struct console * const console, struct server * const server)
{
	memset(console, 0, sizeof(*console));
	console->server = server;
	buffer_init(&console->buffer);
}

void console_free(struct console * const console)
{
	buffer_free(&console->buffer);
}

int start_console(struct console * const console)
{
	sigset_t old, new;

	sigfillset(&new);

	if (pthread_sigmask(SIG_SETMASK, &new, &old))
		goto err;

	if (pthread_create(&console->thread, NULL, console_main, console) != 0)
		goto err;

	if (pthread_sigmask(SIG_SETMASK, &old, NULL))
		goto err_cancel;

	return 0;

err_cancel:
	pthread_cancel(console->thread);
err:
	return -1;
}

void stop_console(struct console * const console)
{
	/*
	 * The asynchronous stuff in libev is the only stuff that is thread-safe,
	 * that's why we need this complexity for killing the console thread.
	 */
	ev_async_send(console->loop, &console->kill_watcher);
}

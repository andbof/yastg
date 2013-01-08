#include <dlfcn.h>
#include <malloc.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <config.h>
#include "console.h"
#include "common.h"
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

static int cmd_ports(void *ptr, char *param)
{
	struct port_type *type;

	mprintf("%-26s %-26s %-8s %-8s %-8s %-8s\n",
			"Name", "Description", "OCEAN", "SURFACE",
			"ORBIT", "ROGUE");
	list_for_each_entry(type, &univ.port_types, list)
		mprintf("%-26.26s %-26.26s %-8s %-8s %-8s %-8s\n",
				type->name, type->desc,
				(type->zones[OCEAN] ? "Yes" : "No"),
				(type->zones[SURFACE] ? "Yes" : "No"),
				(type->zones[ORBIT] ? "Yes" : "No"),
				(type->zones[ROGUE] ? "Yes" : "No"));

	return 0;
}

static int cmd_help(void *_cli_root, char *param)
{
	struct list_head *cli_root = _cli_root;

	cli_print_help(stdout, cli_root);

	return 0;
}

static int cmd_insmod(void *ptr, char *param)
{
	int r;

	if (!param) {
		mprintf("usage: insmod <file name.so>\n");
		return 0;
	}

	r = module_insert(param);
	if (r != 0)
		mprintf("Error inserting module: %s\n",
				(r == MODULE_DL_ERROR ? dlerror() : module_strerror(r))
		       );

	return 0;
}

static int cmd_items(void *ptr, char *param)
{
	struct item *i;

	mprintf("%-24s %-8s\n", "Name", "Weight");
	list_for_each_entry(i, &univ.items, list)
		mprintf("%-24.24s %-8ld\n", i->name, i->weight);

	return 0;
}

static int cmd_lsmod(void *ptr, char *param)
{
	struct module *m;

	if (list_empty(&modules_loaded)) {
		mprintf("No modules are currently loaded\n");
		return 0;
	}

	mprintf("%-16s %-8s\n", "Module", "Size");
	list_for_each_entry(m, &modules_loaded, list)
		mprintf("%-16s %-8d\n", m->name, m->size);

	return 0;
}

static int cmd_wall(void *_server, char *message)
{
	struct server *server = _server;
	if (message) {
		struct signal msg = {
			.cnt = strlen(message) + 1,
			.type = MSG_WALL
		};
		write_msg(server->fd[1], &msg, message);
	} else {
		mprintf("usage: wall <message>\n");
	}
	return 0;
}

static int cmd_pause(void *_server, char *param)
{
	struct server *server = _server;
	struct signal msg = {
		.cnt = 0,
		.type = MSG_PAUSE
	};
	write_msg(server->fd[1], &msg, NULL);
	return 0;
}

static int cmd_planets(void *ptr, char *param)
{
	struct planet_type *type;

	mprintf("%-5s %-26s %-4s %-4s %-4s %-12s %-12s %-10s\n",
			"Class", "Name", "HOT", "ECO", "COLD",
			"Min life", "Max life", "Port types");
	list_for_each_entry(type, &univ.planet_types, list)
		mprintf("%c     %-26.26s %-4.4s %-4.4s %-4.4s %-12.12s %-12.12s %-10lu\n",
				type->c, type->name,
				(type->zones[HOT] ? "Yes" : "No"),
				(type->zones[ECO] ? "Yes" : "No"),
				(type->zones[COLD] ? "Yes" : "No"),
				(planet_life_names[type->minlife]),
				(planet_life_names[type->maxlife]),
				ptrlist_len(&type->port_types));

	return 0;
}

static int cmd_resume(void *_server, char *param)
{
	struct server *server = _server;
	struct signal msg = {
		.cnt = 0,
		.type = MSG_CONT
	};
	write_msg(server->fd[1], &msg, NULL);
	return 0;
}

static void _cmd_rmmod(struct module *m)
{
	int r;

	r = module_remove(m);
	if (r)
		mprintf("Error removing module: %s\n",
				(r == MODULE_DL_ERROR ? dlerror() : module_strerror(r))
		       );
}

static int cmd_rmmod(void *ptr, char *name)
{
	struct module *m;

	if (!name) {
		mprintf("usage: rmmod <module>\n");
		return 0;
	}

	list_for_each_entry(m, &modules_loaded, list) {
		if (strcmp(m->name, name) == 0) {
			_cmd_rmmod(m);
			return 0;
		}
	}

	mprintf("Module %s is not currently loaded\n", name);
	return 0;
}

static int cmd_ships(void *ptr, char *param)
{
	struct ship_type *type;

	mprintf("%-26s %-26s %-12s\n",
			"Name", "Description", "Carry weight");
	list_for_each_entry(type, &univ.ship_types, list)
		mprintf("%-26.26s %-26.26s %-12d\n",
				type->name, type->desc, type->carry_weight);

	return 0;
}

static int cmd_memstat(void *ptr, char *param)
{
	struct mallinfo minfo = mallinfo();
	mprintf("Memory statistics:\n"
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

static int cmd_stats(void *ptr, char *param)
{
	struct tm t;
	char created[32];
	memset(created, 0, sizeof(created));

	localtime_r(&univ.created, &t);
	strftime(created, sizeof(created), "%c", &t);

	mprintf("Statistics:\n"
			"  Size of universe:          %lu systems\n"
			"  Universe created:          %s\n"
			"  Number of users known:     %s\n"
			"  Number of users connected: %s\n",
			ptrlist_len(&univ.systems),
			created,
			"FIXME", "FIXME");
	return 0;
}

static int cmd_quit(void *_server, char *param)
{
	struct server *server = _server;
	mprintf("Bye!\n");
	server->running = 0;
	return 0;
}

static int register_console_commands(struct list_head * const cli_root, struct server * server)
{
	if (cli_add_cmd(cli_root, "ports", cmd_ports, cli_root, "List available ports"))
		goto err;
	if (cli_add_cmd(cli_root, "help", cmd_help, cli_root, "Display this help text"))
		goto err;
	if (cli_add_cmd(cli_root, "insmod", cmd_insmod, NULL, "Insert a loadable module"))
		goto err;
	if (cli_add_cmd(cli_root, "items", cmd_items, NULL, "List available items"))
		goto err;
	if (cli_add_cmd(cli_root, "lsmod", cmd_lsmod, NULL, "List modules currently loaded"))
		goto err;
	if (cli_add_cmd(cli_root, "wall", cmd_wall, server, "Send a message to all connected players"))
		goto err;
	if (cli_add_cmd(cli_root, "pause", cmd_pause, server, "Pause all players"))
		goto err;
	if (cli_add_cmd(cli_root, "planets", cmd_planets, server, "List available planet types"))
		goto err;
	if (cli_add_cmd(cli_root, "resume", cmd_resume, server, "Resume all players"))
		goto err;
	if (cli_add_cmd(cli_root, "rmmod", cmd_rmmod, NULL, "Unload a loadable module"))
		goto err;
	if (cli_add_cmd(cli_root, "ships", cmd_ships, NULL, "List available ship types"))
		goto err;
	if (cli_add_cmd(cli_root, "stats", cmd_stats, NULL, "Display statistics"))
		goto err;
	if (cli_add_cmd(cli_root, "memstat", cmd_memstat, NULL, "Display memory statistics"))
		goto err;
	if (cli_add_cmd(cli_root, "quit", cmd_quit, server, "Terminate the server"))
		goto err;

	return 0;

err:
	cli_tree_destroy(cli_root);
	return -1;
}

int run_console(struct server *server)
{
	char line[256];	/* FIXME */
	LIST_HEAD(cli_root);

	if (register_console_commands(&cli_root, server))
		die("%s", "Could not register console commands");

	mprintf("Welcome to YASTG %s, built %s %s.\n\n", PACKAGE_VERSION, __DATE__, __TIME__);
	mprintf("Universe has %lu systems in total\n", ptrlist_len(&univ.systems));

	while (server->running) {
		mprintf("console> ");
		fgets(line, sizeof(line), stdin); /* FIXME */
		chomp(line);

		if (strlen(line) > 0 && cli_run_cmd(&cli_root, line) < 0)
			mprintf("Unknown command or syntax error.\n");
	}

	cli_tree_destroy(&cli_root);

	return 0;
}

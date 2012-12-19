#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <malloc.h>
#include <dlfcn.h>

#include "config.h"
#include "common.h"
#include "log.h"
#include "mtrandom.h"
#include "cli.h"
#include "ptrlist.h"
#include "server.h"
#include "system.h"
#include "base.h"
#include "inventory.h"
#include "item.h"
#include "player.h"
#include "universe.h"
#include "parseconfig.h"
#include "civ.h"
#include "names.h"
#include "module.h"

#define PORT "2049"
#define BACKLOG 16

const char* options = "d";
int detached = 0;

extern int sockfd;

struct server {
	pthread_t thread;
	int fd[2];
	int running;
};

static void parse_command_line(int argc, char **argv)
{
	char c;
	while ((c = getopt(argc, argv, options)) > 0) {
		switch (c) {
		case 'd':
			printf("Detached mode\n");
			detached = 1;
			break;
		default:
			exit(1);
		}
	}
}

static void write_msg(int fd, struct signal *msg, char *msgdata)
{
	if (write(fd, msg, sizeof(*msg)) < 1)
		bug("%s", "server signalling fd is closed");
	if (msg->cnt && write(fd, msgdata, msg->cnt) < 1)
		bug("%s", "server signalling fd is closed");
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
		mprintf("%-24.24s %-8u\n", i->name, i->weight);

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

static void open_log_file()
{
	log_init("yastg.log");
	log_printfn("main", "YASTG initializing");
	log_printfn("main", "This is %s, built %s %s", PACKAGE_VERSION, __DATE__, __TIME__);
}

static void initialize_server(struct server * const server)
{
	server->running = 1;
	srand(time(NULL));
	mtrandom_init();
}

static int register_console_commands(struct list_head * const cli_root, struct server * server)
{
	if (cli_add_cmd(cli_root, "help", cmd_help, cli_root, "Display this help text"))
		return -1;
	if (cli_add_cmd(cli_root, "insmod", cmd_insmod, NULL, "Insert a loadable module"))
		return -1;
	if (cli_add_cmd(cli_root, "items", cmd_items, NULL, "List available items"))
		return -1;
	if (cli_add_cmd(cli_root, "lsmod", cmd_lsmod, NULL, "List modules currently loaded"))
		return -1;
	if (cli_add_cmd(cli_root, "wall", cmd_wall, server, "Send a message to all connected players"))
		return -1;
	if (cli_add_cmd(cli_root, "pause", cmd_pause, server, "Pause all players"))
		return -1;
	if (cli_add_cmd(cli_root, "resume", cmd_resume, server, "Resume all players"))
		return -1;
	if (cli_add_cmd(cli_root, "rmmod", cmd_rmmod, NULL, "Unload a loadable module"))
		return -1;
	if (cli_add_cmd(cli_root, "stats", cmd_stats, NULL, "Display statistics"))
		return -1;
	if (cli_add_cmd(cli_root, "memstat", cmd_memstat, NULL, "Display memory statistics"))
		return -1;
	if (cli_add_cmd(cli_root, "quit", cmd_quit, server, "Terminate the server"))
		return -1;

	return 0;
}

static int parse_config_files(struct civ * const civs)
{
	printf("Parsing configuration files\n");

	printf("  civilizations: ");
	civ_init(civs);
	if (civ_load_all(civs))
		return -1;

	printf("done, %lu civs loaded.\n", list_len(&civs->list));

	return 0;
}

static int create_universe(struct universe * const u, struct civ * const civs)
{
	printf("Creating universe\n");
	universe_init(u);

	printf("Loading items ... ");
	if (load_all_items(&u->items))
		return -1;
	printf("done, %lu items loaded\n", list_len(&u->items));

	printf("Loading names ... ");
	names_init(&u->avail_base_names);
	names_init(&u->avail_player_names);
	names_load(&u->avail_base_names, "data/placeprefix", "data/placenames", NULL, "data/placesuffix");
	names_load(&u->avail_player_names, NULL, "data/firstnames", "data/surnames", NULL);
	printf("done.\n");

	if (universe_genesis(u, civs))
		return -1;

	return 0;
}

static int start_server_thread(struct server * const server)
{
	if (pipe(server->fd) != 0)
		return -1;
	if (pthread_create(&server->thread, NULL, server_main, &server->fd[0]) != 0)
		return -1;

	return 0;
}

static void kill_server_thread(struct server * const server)
{
	struct signal signal = {
		.cnt = 0,
		.type = MSG_TERM
	};

	if (write(server->fd[1], &signal, sizeof(signal)) < 1)
		bug("%s", "server signalling fd seems closed when sending signal");

	log_printfn("main", "waiting for server to terminate");
	pthread_join(server->thread, NULL);
}

int main(int argc, char **argv)
{
	char *line = malloc(256); /* FIXME */
	struct civ *cv;
	struct system *s;
	struct civ civs;
	struct server server;
	LIST_HEAD(cli_root);

	open_log_file();

	initialize_server(&server);

	parse_command_line(argc, argv);

	if (register_console_commands(&cli_root, &server))
		die("%s", "Could not register console commands");

	if (parse_config_files(&civs))
		die("%s", "Could not parse config files");

	if (create_universe(&univ, &civs))
		die("%s", "Could not create universe");

	if (start_server_thread(&server))
		die("%s", "Could not start server thread");

	mprintf("Welcome to YASTG %s, built %s %s.\n\n", PACKAGE_VERSION, __DATE__, __TIME__);
	mprintf("Universe has %lu systems in total\n", ptrlist_len(&univ.systems));

	while (server.running) {
		mprintf("console> ");
		fgets(line, 256, stdin); /* FIXME */
		chomp(line);

		if (strlen(line) > 0 && cli_run_cmd(&cli_root, line) < 0)
			mprintf("Unknown command or syntax error.\n");
	}

	/*
	 * This will also automatically kill all player threads and terminate all connections
	 * */
	kill_server_thread(&server);

	log_printfn("main", "cleaning up");
	mprintf("Cleaning up ... ");

	struct list_head *p, *q;
	ptrlist_for_each_entry(s, &univ.systems, p) {
		system_free(s);
	}

	list_for_each_safe(p, q, &civs.list) {
		cv = list_entry(p, struct civ, list);
		list_del(p);
		civ_free(cv);
		free(cv);
	}
	civ_free(&civs);

	names_free(&univ.avail_base_names);
	names_free(&univ.avail_player_names);

	universe_free(&univ);
	free(line);
	cli_tree_destroy(&cli_root);
	log_close();

	printf("done.\n");

	return 0;
}

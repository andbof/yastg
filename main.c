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

#include "defines.h"
#include "log.h"
#include "mtrandom.h"
#include "cli.h"
#include "sarray.h"
#include "ptrlist.h"
#include "test.h"
#include "server.h"
#include "sector.h"
#include "base.h"
#include "inventory.h"
#include "player.h"
#include "universe.h"
#include "parseconfig.h"
#include "civ.h"
#include "id.h"

#define PORT "2049"
#define BACKLOG 16

const char* options = "d";
int detached = 0;
struct cli_tree *cli_root;

extern int sockfd;

struct server {
	pthread_t thread;
	int fd[2];
	int running;
};

static void parseopts(int argc, char **argv)
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

/*
 * Removes trailing newlines from a string, if any exists.
 */
static void chomp(char* s)
{
	for (unsigned int len = strlen(s); len > 0 && s[len - 1] == '\n'; s[len - 1] = '\0', len--);
}

static void write_msg(int fd, struct signal *msg, char *msgdata)
{
	if (write(fd, msg, sizeof(*msg)) < 1)
		bug("%s", "server signalling fd is closed");
	if (msg->cnt && write(fd, msgdata, msg->cnt) < 1)
		bug("%s", "server signalling fd is closed");
}

static int cmd_help(void *ptr, char *param)
{
	cli_print_help(stdout, cli_root);
	return 0;
}

static int cmd_wall(void *ptr, char *param)
{
	struct server *server = ptr;
	if (param) {
		struct signal msg = {
			.cnt = strlen(param) + 1,
			.type = MSG_WALL
		};
		write_msg(server->fd[1], &msg, param);
	} else {
		mprintf("usage: wall <message>\n");
	}
	return 0;
}

static int cmd_pause(void *ptr, char *param)
{
	struct server *server = ptr;
	struct signal msg = {
		.cnt = 0,
		.type = MSG_PAUSE
	};
	write_msg(server->fd[1], &msg, NULL);
	return 0;
}

static int cmd_resume(void *ptr, char *param)
{
	struct server *server = ptr;
	struct signal msg = {
		.cnt = 0,
		.type = MSG_CONT
	};
	write_msg(server->fd[1], &msg, NULL);
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
	mprintf("Statistics:\n"
	        "  Size of universe:          %lu sectors\n"
	        "  Number of users known:     %s\n"
	        "  Number of users connected: %s\n",
		ptrlist_len(univ->sectors), "FIXME", "FIXME");
	return 0;
}

static int cmd_quit(void *ptr, char *param)
{
	struct server *server = ptr;
	mprintf("Bye!\n");
	server->running = 0;
	return 0;
}

int main(int argc, char **argv)
{
	char *line = malloc(256); /* FIXME */
	struct configtree *ctree;
	struct civ *cv;
	struct sector *s, *t;
	size_t st, su;
	struct civ *civs = civ_create();
	struct server server;

	/* Open log file */
	log_init();
	log_printfn("main", "YASTG initializing");
	log_printfn("main", "v%s (commit %s), built %s %s", QUOTE(__VER__), QUOTE(__COMMIT__), __DATE__, __TIME__);

	/* Initialize */
	server.running = 1;
	srand(time(NULL));
	mtrandom_init();
	init_id();

#ifdef TEST
	run_tests();
#endif

	/* Parse command line options */
	parseopts(argc, argv);

	/* Register server console commands */
	printf("Registering server console commands\n");
	cli_root = cli_tree_create();
	if (cli_root == NULL)
		die("%s", "Unable to allocate memory\n");
	cli_add_cmd(cli_root, "help", cmd_help, &server, NULL);
	cli_add_cmd(cli_root, "wall", cmd_wall, &server, NULL);
	cli_add_cmd(cli_root, "pause", cmd_pause, &server, NULL);
	cli_add_cmd(cli_root, "resume", cmd_resume, &server, NULL);
	cli_add_cmd(cli_root, "stats", cmd_stats, &server, NULL);
	cli_add_cmd(cli_root, "memstat", cmd_memstat, &server, NULL);
	cli_add_cmd(cli_root, "quit", cmd_quit, &server, NULL);

	/* Load config files */
	printf("Parsing configuration files\n");
	printf("  civilizations: ");
	civ_load_all(civs);
	printf("done, %lu civs loaded.\n", list_len(&civs->list));
	/* Create universe */
	printf("Creating universe\n");
	univ = universe_create();

	universe_init(civs);

	/* Start server thread */
	if (pipe(server.fd) != 0)
		die("%s", "Could not create server pipe");
	if (pthread_create(&server.thread, NULL, server_main, &server.fd[0]) != 0)
		die("%s", "Could not launch server thread");

	mprintf("Welcome to YASTG v%s (commit %s), built %s %s.\n\n", QUOTE(__VER__), QUOTE(__COMMIT__), __DATE__, __TIME__);
	mprintf("Universe has %lu sectors in total\n", ptrlist_len(univ->sectors));

	while (server.running) {
		mprintf("console> ");
		fgets(line, 256, stdin); /* FIXME */
		chomp(line);

		if (strlen(line) > 0 && cli_run_cmd(cli_root, line) < 0)
			mprintf("Unknown command or syntax error.\n");
	}

	/* Kill server thread, this will also kill all player threads */
	struct signal signal = {
		.cnt = 0,
		.type = MSG_TERM
	};
	if (write(server.fd[1], &signal, sizeof(signal)) < 1)
		bug("%s", "server signalling fd seems closed when sending signal");
	log_printfn("main", "waiting for server to terminate");
	pthread_join(server.thread, NULL);

	/* Destroy all structures and free all memory */

	log_printfn("main", "cleaning up");
	struct list_head *p, *q;
	ptrlist_for_each_entry(s, univ->sectors, p) {
		printf("Freeing sector %s\n", s->name);
		sector_free(s);
	}
	list_for_each_safe(p, q, &civs->list) {
		cv = list_entry(p, struct civ, list);
		list_del(p);
		civ_free(cv);
	}
	civ_free(civs);

	id_destroy();
	universe_free(univ);
	free(line);
	cli_tree_destroy(cli_root);
	log_close();

	return 0;

}

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
#include "planet.h"
#include "base.h"
#include "inventory.h"
#include "player.h"
#include "universe.h"
#include "parseconfig.h"
#include "civ.h"
#include "id.h"
#include "star.h"

#define PORT "2049"
#define BACKLOG 16

const char* options = "d";
int detached = 0;
int running;
struct cli_tree *cli_root;

extern int sockfd;

static pthread_t srvthread;
static int srvfd[2];

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

static int cmd_help(void *ptr, char *param)
{
	cli_print_help(stdout, cli_root);
	return 0;
}

static int cmd_wall(void *ptr, char *param)
{
	if (param) {
		int i = MSG_WALL;
		size_t st = (size_t)param;
		write(srvfd[1], &i, sizeof(i));
		write(srvfd[1], &st, sizeof(st));
	} else {
		mprintf("usage: wall <message>\n");
	}
	return 0;
}

static int cmd_pause(void *ptr, char *param)
{
	int i = MSG_PAUSE;
	size_t st = 0;
	write(srvfd[1], &i, sizeof(i));
	write(srvfd[1], &st, sizeof(st));
	return 0;
}

static int cmd_resume(void *ptr, char *param)
{
	int i = MSG_CONT;
	size_t st = 0;
	write(srvfd[1], &i, sizeof(i));
	write(srvfd[1], &st, sizeof(st));
	return 0;
}

static int cmd_memstat(void *ptr, char *param)
{
	struct mallinfo minfo = mallinfo();
	mprintf("Memory statistics:\n");
	mprintf("  Memory allocated with sbrk by malloc:           %d bytes\n", minfo.arena);
	mprintf("  Number of chunks not in use:                    %d\n", minfo.ordblks);
	mprintf("  Number of chunks allocated with mmap:           %d\n", minfo.hblks);
	mprintf("  Memory allocated with mmap:                     %d bytes\n", minfo.hblkhd);
	mprintf("  Memory occupied by chunks handed out by malloc: %d bytes\n", minfo.uordblks);
	mprintf("  Memory occupied by free chunks:                 %d bytes\n", minfo.fordblks);
	mprintf("  Size of top-most releasable chunk:              %d bytes\n", minfo.keepcost);
	return 0;
}

static int cmd_stats(void *ptr, char *param)
{
	mprintf("Statistics:\n");
	mprintf("  Size of universe:          %lu sectors\n", ptrlist_len(univ->sectors));
	mprintf("  Number of users known:     %s\n", "FIXME");
	mprintf("  Number of users connected: %s\n", "FIXME");
	return 0;
}

static int cmd_quit(void *ptr, char *param)
{
	mprintf("Bye!\n");
	running = 0;
	return 0;
}

int main(int argc, char **argv)
{
	int i;
	char *line = malloc(256); /* FIXME */
	struct configtree *ctree;
	struct civ *cv;
	struct sector *s, *t;
	struct planet *pl;
	struct star *sol;
	size_t st, su;
	struct sarray *gurka;
	unsigned int tomat;
	struct civ *civs = civ_create();

	/* Open log file */
	log_init();
	log_printfn("main", "YASTG initializing");
	log_printfn("main", "v%s (commit %s), built %s %s", QUOTE(__VER__), QUOTE(__COMMIT__), __DATE__, __TIME__);

	/* Initialize */
	running = 1;
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
	cli_add_cmd(cli_root, "help", cmd_help, NULL, NULL);
	cli_add_cmd(cli_root, "wall", cmd_wall, NULL, NULL);
	cli_add_cmd(cli_root, "pause", cmd_pause, NULL, NULL);
	cli_add_cmd(cli_root, "resume", cmd_resume, NULL, NULL);
	cli_add_cmd(cli_root, "stats", cmd_stats, NULL, NULL);
	cli_add_cmd(cli_root, "memstat", cmd_memstat, NULL, NULL);
	cli_add_cmd(cli_root, "quit", cmd_quit, NULL, NULL);

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
	if (pipe(srvfd) != 0)
		die("%s", "Could not create server pipe");
	if (pthread_create(&srvthread, NULL, server_main, &srvfd[0]) != 0)
		die("%s", "Could not launch server thread");

	mprintf("Welcome to YASTG v%s (commit %s), built %s %s.\n\n", QUOTE(__VER__), QUOTE(__COMMIT__), __DATE__, __TIME__);
	mprintf("Universe has %lu sectors in total\n", ptrlist_len(univ->sectors));

	while (running) {
		mprintf("console> ");
		fgets(line, 256, stdin); /* FIXME */
		chomp(line);

		if (cli_run_cmd(cli_root, line) < 0)
			mprintf("Unknown command or syntax error.\n");
	}

	/* Kill server thread, this will also kill all player threads */
	enum msg m = MSG_TERM;
	st = 0;
	if (write(srvfd[1], &m, sizeof(m)) < 1)
		bug("%s", "server signalling fd seems closed when sending signal");
	if (write(srvfd[1], &st, sizeof(st)) < 1)
		bug("%s", "server signalling fd seems closed when sending parameter");
	log_printfn("main", "waiting for server to terminate");
	pthread_join(srvthread, NULL);

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

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
#include "loadconfig.h"
#include "console.h"
#include "log.h"
#include "mtrandom.h"
#include "cli.h"
#include "ptrlist.h"
#include "server.h"
#include "system.h"
#include "port.h"
#include "port_type.h"
#include "inventory.h"
#include "item.h"
#include "player.h"
#include "planet.h"
#include "planet_type.h"
#include "ship_type.h"
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

static int parse_command_line(int argc, char **argv)
{
	char c;
	while ((c = getopt(argc, argv, options)) > 0) {
		switch (c) {
		case 'd':
			printf("Detached mode\n");
			detached = 1;
			break;
		default:
			return -1;
		}
	}

	return 0;
}

static void open_log_file()
{
	log_init("yastg.log");
	log_printfn(LOG_MAIN, "YASTG initializing");
	log_printfn(LOG_MAIN, "This is %s, built %s %s", PACKAGE_VERSION, __DATE__, __TIME__);
}

static int create_universe(struct universe * const u)
{
	printf("Creating universe\n");

	if (universe_genesis(u))
		return -1;

	return 0;
}

static int wait_for_exit()
{
	sigset_t set;
	int signal;

	if (sigemptyset(&set))
		goto err;
	if (sigaddset(&set, SIGHUP))
		goto err;
	if (sigaddset(&set, SIGINT))
		goto err;
	if (sigaddset(&set, SIGTERM))
		goto err;

	if (pthread_sigmask(SIG_BLOCK, &set, NULL))
		goto err;
	if (sigwait(&set, &signal))
		goto err;

	log_printfn(LOG_MAIN, "process received signal %d", signal);
	return 0;

err:
	return -1;
}

int main(int argc, char **argv)
{
	struct server server;
	struct console console;

	open_log_file();

	if (parse_command_line(argc, argv))
		die("%s", "Syntax error on command line");

	srand(time(NULL));
	mtrandom_init();

	universe_init(&univ);
	names_init(&univ.avail_constellations);
	names_init(&univ.avail_port_names);
	names_init(&univ.avail_player_names);

	if (parse_config_files(&univ))
		die("%s", "Could not parse config files");

	if (create_universe(&univ))
		die("%s", "Could not create universe");

	if (start_server(&server))
		die("%s", "Could not start server thread");

	console_init(&console, &server);
	if (start_console(&console))
		die("%s", "Could not start console thread");

	if (wait_for_exit())
		die("%s", "Signal handling error");

	/*
	 * This will also automatically kill all player threads and terminate all connections
	 */
	stop_console(&console);
	stop_server(&server);
	pthread_join(console.thread, NULL);
	pthread_join(server.thread, NULL);

	log_printfn(LOG_MAIN, "cleaning up");
	printf("Cleaning up ... ");

	console_free(&console);

	struct list_head *lh;
	struct system *s;
	ptrlist_for_each_entry(s, &univ.systems, lh) {
		system_free(s);
	}

	struct civ *cv, *_cv;
	list_for_each_entry_safe(cv, _cv, &univ.civs, list) {
		list_del(&cv->list);
		civ_free(cv);
		free(cv);
	}

	names_free(&univ.avail_constellations);
	names_free(&univ.avail_port_names);
	names_free(&univ.avail_player_names);

	universe_free(&univ);
	log_close();

	printf("done.\n");

	return 0;
}

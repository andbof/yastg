#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "common.h"
#include "log.h"

#define CTIME_LEN 26		/* From ctime() */

static pthread_mutex_t log_mutex;
static FILE *log_fd;
static char log_timestr[CTIME_LEN];

static const char subsystems[LOG_SUBSYSTEM_NUM][16] = {
	"config",
	"connection",
	"main",
	"panic",
	"port_update",
	"server"
};

void log_init(const char * const name)
{
	if (pthread_mutex_init(&log_mutex, NULL) != 0)
		die("%s", "Failed initializing log mutex");
	if ((log_fd = fopen(name, "a+")) == NULL)
		die("Failed creating or opening log file %s", name);
}

void log_init_stdout()
{
	if (pthread_mutex_init(&log_mutex, NULL) != 0)
		die("%s", "Failed initializing log mutex");
	log_fd = stdout;
}

void log_close()
{
	pthread_mutex_lock(&log_mutex);
	if (log_fd != stdout)
		fclose(log_fd);
	log_fd = NULL;
	pthread_mutex_unlock(&log_mutex);
	pthread_mutex_destroy(&log_mutex);
}

void __attribute__((format(printf, 2, 3))) log_printfn(const enum log_subsystems subsystem,
		const char *format, ...)
{
	/* We don't check the return codes from the fprintf() or the fflush(),
	   this is a feature. */
	va_list ap;
	time_t now = time(NULL);
	int i = -1;

	assert(subsystem < LOG_SUBSYSTEM_NUM);

	if (!log_fd)
		return;

	pthread_mutex_lock(&log_mutex);

	ctime_r(&now, &log_timestr[0]);
	while (log_timestr[++i] != '\n');
	log_timestr[i] = '\0';

	fprintf(log_fd, "%s %s: ", log_timestr, subsystems[subsystem]);
	va_start(ap, format);
	vfprintf(log_fd, format, ap);
	va_end(ap);
	fprintf(log_fd, "%s", "\n");
	fflush(log_fd);

	pthread_mutex_unlock(&log_mutex);
}

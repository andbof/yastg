#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>
#include <ev.h>

#include "common.h"
#include "log.h"
#include "connection.h"
#include "server.h"
#include "ptrlist.h"
#include "cli.h"
#include "universe.h"
#include "sector.h"
#include "star.h"
#include "base.h"
#include "planet.h"
#include "player.h"
#include "mtrandom.h"

extern struct universe *univ;

struct connection* conn_create()
{
	struct connection *data;
	int r = 1;
	data = malloc(sizeof(*data));
	if (data != NULL) {
		memset(data, 0, sizeof(*data));
		data->id = mtrandom_uint(UINT32_MAX);
		data->rbufs = CONN_BUFSIZE;
		if ((data->rbuf = malloc(data->rbufs)) == NULL) {
			log_printfn("connection", "failed allocating receive buffer for connection");
			connection_free(data);
			return NULL;
		}
		data->sbufs = CONN_MAXBUFSIZE;
		if ((data->sbuf = malloc(data->sbufs)) == NULL) {
			log_printfn("connection", "failed allocating send buffer for connection");
			connection_free(data);
			return NULL;
		}
		INIT_LIST_HEAD(&data->list);
	}
	return data;
}

static void conn_signalserver(struct connection *data, struct signal *msg, char *msgdata)
{
	if (write(data->serverfd, msg, sizeof(*msg)) < 1)
		bug("server signalling fd seems closed when sending signal: error %d (%s)", errno, strerror(errno));
	if (msg->cnt > 0) {
		if (write(data->serverfd, msgdata, msg->cnt) < msg->cnt)
			bug("server signalling fd seems closed when seems closed when sending data: error %d (%s)", errno, strerror(errno));
	}
}

/*
 * This function needs to be very safe as it can be called on a
 * half-initialized connection structure if something went wrong.
 */
void connection_free(void *ptr)
{
	struct connection *data = ptr;
	if (data != NULL) {
		if (data->peerfd)
			close(data->peerfd);
		if (data->rbuf)
			free(data->rbuf);
		if (data->sbuf)
			free(data->sbuf);
		if (data->pl)
			player_free(data->pl);
	}
}

void conn_cleanexit(struct connection *data)
{
	log_printfn("connection", "connection %x is terminating", data->id);
	struct signal msg = {
		.cnt = sizeof(data->id),
		.type = MSG_RM,
	};
	conn_signalserver(data, &msg, (char*)&data->id);
}

void conn_send(struct connection *data, char *format, ...)
{
	va_list ap;
	size_t len;

	va_start(ap, format);
	len = vsnprintf(data->sbuf, data->sbufs, format, ap);
	if (len >= data->sbufs)
		log_printfn("connection", "warning: send buffer overflow on connection %x (wanted to send %zu bytes but buffer size is %zu bytes), truncating data", data->id, len, data->sbufs);
	va_end(ap);

	size_t sb = 0;
	int i;
	len = strlen(data->sbuf);
	do {
		i = send(data->peerfd, data->sbuf + sb, len - sb, MSG_NOSIGNAL);
		if (i < 1) {
			log_printfn("connection", "send error (connection %x), terminating connection", data->id);
			conn_cleanexit(data);
		}
		sb += i;
	} while (sb < len);

}

void conn_error(struct connection *data, char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	conn_send(data, "Oops! An internal error occured: %s.\nYour current state is NOT saved and you are being forcibly disconnected.\nSorry!", format, ap);
	va_end(ap);
	conn_cleanexit(data);
}

#define PROMPT "yastg> "
void* connection_worker(void *_w)
{
	struct conn_worker_list *w = _w;
	struct conn_data *data = w->conn_data;
	struct connection *conn;

	do {
		pthread_mutex_lock(&data->workers_lock);

		while (ptrlist_empty(&data->work_items) && !w->terminate)
			pthread_cond_wait(&data->workers_cond, &data->workers_lock);
		conn = ptrlist_pull(&data->work_items);

		pthread_mutex_unlock(&data->workers_lock);

		if (w->terminate)
			break;

		if (conn->rbuf[0] != '\0' && cli_run_cmd(&conn->pl->cli, conn->rbuf) < 0)
			conn_send(conn, "Unknown command or syntax error: \"%s\"\n", conn->rbuf);
		conn_send(conn, PROMPT);
	} while(1);

	return NULL;
}

void conn_fulfixinit(struct connection *data)
{
	MALLOC_DIE(data->pl, sizeof(*data->pl));
	player_init(data->pl);

	log_printfn("connection", "peer %s successfully logged in as %s", data->peer, data->pl->name);

	data->pl->conn = data;
	player_go(data->pl, SECTOR, ptrlist_entry(&univ->sectors, 0));

	conn_send(data, PROMPT);
}

void conn_do_work(struct conn_data *data, struct connection *conn)
{
	pthread_mutex_lock(&data->workers_lock);
	ptrlist_push(&data->work_items, conn);
	pthread_cond_signal(&data->workers_cond);
	pthread_mutex_unlock(&data->workers_lock);
}

static int start_new_worker(struct conn_data *data)
{
	int r;
	struct conn_worker_list *w;
	w = malloc(sizeof(*w));
	if (!w)
		return -1;
	memset(w, 0, sizeof(*w));
	w->conn_data = data;

	r = pthread_create(&w->thread, NULL, connection_worker, w);
	if (r) {
		free(w);
		return -1;
	}

	list_add(&w->list, &data->workers);

	return 0;
}

#define NUM_WORKERS 4
static int initialize_workers(struct conn_data *data)
{
	INIT_LIST_HEAD(&data->workers);

	for (int i = 0; i < NUM_WORKERS; i++) {
		if (start_new_worker(data))
			goto err;
	}

	return 0;

	struct conn_worker_list *w;
err:
	list_for_each_entry(w, &data->workers, list)
		free(w);

	return -1;
}       

int conn_init(struct conn_data *data)
{
	memset(data, 0, sizeof(*data));
	if (pthread_mutex_init(&data->workers_lock, NULL))
		return -1;
	if (pthread_cond_init(&data->workers_cond, NULL))
		goto err_mutex;
	if (initialize_workers(data))
		goto err_cond;
	if (ptrlist_init(&data->work_items))
		goto err_workers;

	return 0;

err_workers:
	ptrlist_free(&data->work_items);
err_cond:
	pthread_cond_destroy(&data->workers_cond);
err_mutex:
	pthread_mutex_destroy(&data->workers_lock);

	return -1;
}

void conn_shutdown(struct conn_data *data)
{
	struct conn_worker_list *w;

	pthread_mutex_lock(&data->workers_lock);

	list_for_each_entry(w, &data->workers, list)
		w->terminate = 1;
	pthread_cond_broadcast(&data->workers_cond);

	pthread_mutex_unlock(&data->workers_lock);

	list_for_each_entry(w, &data->workers, list)
		pthread_join(w->thread, NULL);
}

void conn_destroy(struct conn_data *data)
{
	pthread_cond_destroy(&data->workers_cond);
	pthread_mutex_destroy(&data->workers_lock);

	struct conn_worker_list *w, *p;
	list_for_each_entry_safe(w, p, &data->workers, list)
		free(w);
}

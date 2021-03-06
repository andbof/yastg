#include <assert.h>
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
#include "buffer.h"
#include "log.h"
#include "connection.h"
#include "server.h"
#include "ptrlist.h"
#include "cli.h"
#include "universe.h"
#include "system.h"
#include "star.h"
#include "port.h"
#include "planet.h"
#include "player.h"
#include "mtrandom.h"

int conn_init(struct connection *conn)
{
	assert(conn);

	memset(conn, 0, sizeof(*conn));
	pthread_mutex_init(&conn->worker_lock, NULL);
	conn->id = mtrandom_uint(UINT32_MAX);
	buffer_init(&conn->send);
	buffer_init(&conn->recv);

	INIT_LIST_HEAD(&conn->list);
	INIT_LIST_HEAD(&conn->work);

	return 0;
}

/*
 * This function needs to be very safe as it can be called on a
 * half-initialized connection structure if something went wrong.
 */
void connection_free(struct connection *conn)
{
	if (!conn)
		return;

	pthread_mutex_destroy(&conn->worker_lock);

	if (conn->peerfd)
		close(conn->peerfd);
	buffer_free(&conn->send);
	buffer_free(&conn->recv);
	if (conn->pl)
		player_free(conn->pl);
}

__attribute__((format(printf, 2, 3)))
void conn_error(struct connection *data, char *fmt, ...)
{
	char msg[128];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);
	msg[sizeof(msg) - 1] = '\0';

	conn_send(data, "Oops! An internal error occured: %s.\nYour current state is NOT saved and you are being forcibly disconnected.\nSorry!", msg);
	server_disconnect_nicely(data);
}

#define PROMPT "yastg> "
void* connection_worker(void *_w)
{
	struct conn_worker_list *w = _w;
	struct conn_data *data = w->conn_data;
	struct connection *conn;

	do {
		/*
		 * Remember: The locking here need to be synchronized with
		 * the locking in disconnect_peer() to avoid nasty races
		 * in case a connection is terminated when work is about to start.
		 */
		pthread_mutex_lock(&data->workers_lock);

		while (list_empty(&data->work_items) && !w->terminate)
			pthread_cond_wait(&data->workers_cond, &data->workers_lock);

		if (w->terminate) {
			pthread_mutex_unlock(&data->workers_lock);
			break;
		}

		conn = list_first_entry(&data->work_items, struct connection, work);
		list_del_init(&conn->work);

		/*
		 * This needs to be in this order to avoid a race when
		 * server_disconnect_cb() is running _now_.
		 */
		pthread_mutex_lock(&conn->worker_lock);
		pthread_mutex_unlock(&data->workers_lock);

		if (conn->terminate) {
			pthread_mutex_unlock(&conn->worker_lock);
			continue;
		}

		if (conn->worker) {
			pthread_mutex_unlock(&conn->worker_lock);
			log_printfn(LOG_CONN, "Got new data too fast -- old worker is still busy");
			continue;
		}
		conn->worker = 1;
		pthread_mutex_unlock(&conn->worker_lock);

		if (conn->recv.buf[0] != '\0' && cli_run_cmd(&conn->pl->cli, conn->recv.buf) < 0)
			conn_send(conn, "Unknown command or syntax error: \"%s\"\n", conn->recv.buf);
		buffer_reset(&conn->recv);
		conn_send(conn, PROMPT);

		pthread_mutex_lock(&conn->worker_lock);
		conn->worker = 0;
		pthread_mutex_unlock(&conn->worker_lock);

	} while(1);

	return NULL;
}

int conn_fulfixinit(struct connection *data)
{
	log_printfn(LOG_CONN, "initializing connection from peer %s", data->peer);

	if (list_len(&univ.ship_types) == 0)
		return -1;

	data->pl = malloc(sizeof(*data->pl));
	if (!data->pl)
		return -1;

	if (player_init(data->pl))
		return -1;

	data->pl->conn = data;
	if (new_ship_to_player(list_first_entry(&univ.ship_types, struct ship_type, list), data->pl))
		return -1;
	data->pl->pos = list_first_entry(&data->pl->ships, struct ship, list);
	data->pl->postype = SHIP;
	data->pl->credits = 100000;

	player_go(data->pl, SYSTEM, ptrlist_entry(&univ.systems, 0));

	conn_send(data, PROMPT);

	log_printfn(LOG_CONN, "peer %s successfully logged in as %s", data->peer, data->pl->name);

	return 0;
}

void conn_do_work(struct conn_data *data, struct connection *conn)
{
	pthread_mutex_lock(&data->workers_lock);
	list_add_tail(&conn->work, &data->work_items);
	pthread_cond_signal(&data->workers_cond);
	pthread_mutex_unlock(&data->workers_lock);
}

void conn_send_buffer(struct connection * const data)
{
	assert(data);
	assert(data->peerfd);
	if (write_buffer_into_fd(data->peerfd, &data->send) < 1) {
		log_printfn(LOG_CONN,
				"send error (connection %x), terminating connection",
				data->id);
		server_disconnect_nicely(data);
	}
}

static int start_new_worker(struct conn_data *data)
{
	struct conn_worker_list *w;
	sigset_t old, new;

	sigfillset(&new);

	w = malloc(sizeof(*w));
	if (!w)
		return -1;
	memset(w, 0, sizeof(*w));
	w->conn_data = data;

	if (pthread_sigmask(SIG_SETMASK, &new, &old))
		goto err_free;

	if (pthread_create(&w->thread, NULL, connection_worker, w))
		goto err_free;

	if (pthread_sigmask(SIG_SETMASK, &old, NULL)) {
		pthread_cancel(w->thread);
		goto err_free;
	}

	list_add(&w->list, &data->workers);

	return 0;

err_free:
	free(w);
	return -1;
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

int conndata_init(struct conn_data *data)
{
	memset(data, 0, sizeof(*data));
	INIT_LIST_HEAD(&data->work_items);

	if (pthread_mutex_init(&data->workers_lock, NULL))
		return -1;
	if (pthread_cond_init(&data->workers_cond, NULL))
		goto err_mutex;
	if (initialize_workers(data))
		goto err_cond;

	return 0;

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

__attribute__((format(printf, 2, 3)))
void conn_send(void *_conn, const char *fmt, ...)
{
	struct connection *conn = _conn;
	va_list ap;
	assert(conn);

	if (conn->terminate)
		return;

	va_start(ap, fmt);
	vbufprintf(&conn->send, fmt, ap);
	va_end(ap);

	conn_send_buffer(conn);
}

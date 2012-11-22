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

struct conndata* conn_create()
{
	struct conndata *data;
	int r = 1;
	data = malloc(sizeof(*data));
	if (data != NULL) {
		memset(data, 0, sizeof(*data));
		data->id = mtrandom_uint(UINT32_MAX);
		data->rbufs = CONN_BUFSIZE;
		if ((data->rbuf = malloc(data->rbufs)) == NULL) {
			log_printfn("connection", "failed allocating receive buffer for connection");
			conndata_free(data);
			return NULL;
		}
		data->sbufs = CONN_MAXBUFSIZE;
		if ((data->sbuf = malloc(data->sbufs)) == NULL) {
			log_printfn("connection", "failed allocating send buffer for connection");
			conndata_free(data);
			return NULL;
		}
		if (pipe(data->threadfds) != 0) {
			log_printfn("connection", "failed opening pipe to thread for connection");
			conndata_free(data);
			return NULL;
		}
		INIT_LIST_HEAD(&data->list);
	}
	return data;
}

static void conn_signalserver(struct conndata *data, struct signal *msg, char *msgdata)
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
 * half-initialized conndata structure if something went wrong.
 */
void conndata_free(void *ptr)
{
	struct conndata *data = ptr;
	if (data != NULL) {
		if (data->peerfd)
			close(data->peerfd);
		if (data->threadfds[0])
			close(data->threadfds[0]);
		if (data->threadfds[1])
			close(data->threadfds[1]);
		if (data->peer)
			free(data->peer);
		if (data->rbuf)
			free(data->rbuf);
		if (data->sbuf)
			free(data->sbuf);
		if (data->pl)
			player_free(data->pl);
	}
}

void conn_cleanexit(struct conndata *data)
{
	log_printfn("connection", "connection %x is terminating", data->id);
	struct signal msg = {
		.cnt = sizeof(data->id),
		.type = MSG_RM,
	};
	conn_signalserver(data, &msg, (char*)&data->id);
	pthread_exit(0);
}

void conn_send(struct conndata *data, char *format, ...)
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

void conn_error(struct conndata *data, char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	conn_send(data, "Oops! An internal error occured: %s.\nYour current state is NOT saved and you are being forcibly disconnected.\nSorry!", format, ap);
	va_end(ap);
	conn_cleanexit(data);
}

static void conn_act(struct conndata *data)
{
	struct sector *s;
	if (data->rbuf[0] != '\0' && cli_run_cmd(data->pl->cli, data->rbuf) < 0)
		conn_send(data, "Unknown command or syntax error: \"%s\"\n", data->rbuf);
}

static void conn_handle_signal(struct conndata *data, struct signal *msg, char *msgdata)
{
	switch (msg->type) {
	case MSG_TERM:
		conn_send(data, "\nServer is shutting down, you are being disconnected.\n");
		conn_cleanexit(data);
		break;
	case MSG_PAUSE:
		conn_send(data, "\nYou have been paused by God. This might mean the whole universe is currently on hold\n"
				"or just you. Anything you enter at the prompt will queue up until you are resumed.\n");
		data->paused = 1;
		break;
	case MSG_CONT:
		conn_send(data, "\nYou have been resumed, feel free to play away!\n");
		data->paused = 0;
		break;
	case MSG_WALL:
		conn_send(data, "\nMessage to all connected users:\n"
				"%s"
				"\nEnd of message.\n", msgdata);
		break;
	default:
		log_printfn("connection", "received unknown signal: %d", msg->type);
	}
}

static void conn_receive_signal(struct conndata *data)
{
	struct signal msg;
	char *msgdata;
	int fd = data->threadfds[0];

	read(fd, &msg, sizeof(msg));
	if (msg.cnt > 0) {
		msgdata = alloca(msg.cnt);
		read(fd, msgdata, msg.cnt);
	} else {
		msgdata = NULL;
	}
	conn_handle_signal(data, &msg, msgdata);
}

static void conn_receive_data(struct conndata *data)
{
	void *ptr;

	data->rbufi += recv(data->peerfd, data->rbuf + data->rbufi, data->rbufs - data->rbufi, 0);
	if (data->rbufi< 1) {
		log_printfn("connection", "peer %s disconnected, terminating connection %x", data->peer, data->id);
		conn_cleanexit(data);
	}
	if ((data->rbufi == data->rbufs) && (data->rbuf[data->rbufi - 1] != '\n')) {
		if (data->rbufs == CONN_MAXBUFSIZE) {
			log_printfn("connection", "peer sent more data than allowed (%u), connection %x terminated", CONN_MAXBUFSIZE, data->id);
			conn_cleanexit(data);
		}
		data->rbufs <<= 1;
		if (data->rbufs > CONN_MAXBUFSIZE)
			data->rbufs = CONN_MAXBUFSIZE;
		if ((ptr = realloc(data->rbuf, data->rbufs)) == NULL) {
			data->rbufs >>= 1;
			log_printfn("connection", "unable to increase receive buffer size, connection %x terminated", data->id);
			conn_cleanexit(data);
		} else {
			data->rbuf = ptr;
		}
	}

	if (data->rbufi != 0 && data->rbuf[data->rbufi - 1] == '\n') {
		data->rbuf[data->rbufi - 1] = '\0';
		chomp(data->rbuf);

		mprintf("debug: received \"%s\" on socket\n", data->rbuf);

		conn_act(data);

		data->rbufi = 0;
		conn_send(data, "yastg> ");
	}

}

static void conn_peer_cb(struct ev_loop *loop, ev_io *w, int revents)
{
	struct conndata *data = w->data;
	conn_receive_data(data);
}

static void conn_server_cb(struct ev_loop *loop, ev_io *w, int revents)
{

	struct conndata *data = w->data;
	conn_receive_signal(data);
}

static void conn_loop(struct conndata *data)
{
	int i;
	char* ptr;
	ev_io peer_watcher, server_watcher;
	struct ev_loop *loop = ev_loop_new(EVFLAG_AUTO);

	peer_watcher.data = data;
	server_watcher.data = data;

	ev_io_init(&server_watcher, conn_server_cb, data->threadfds[0], EV_READ);
	ev_io_init(&peer_watcher, conn_peer_cb, data->peerfd, EV_READ);

	ev_io_start(loop, &server_watcher);
	ev_io_start(loop, &peer_watcher);

	log_printfn("connection", "serving new connection %x", data->id);
	conn_send(data, "yastg> ");

	ev_run(loop, 0);

	ev_io_stop(loop, &peer_watcher);
	ev_io_stop(loop, &server_watcher);

	ev_loop_destroy(loop);
}

void* conn_main(void *dataptr)
{
	struct conndata *data = dataptr;
	/* temporary solution */
	/* Create player */
	MALLOC_DIE(data->pl, sizeof(*data->pl));
	player_init(data->pl);

	log_printfn("connection", "peer %s successfully logged in as %s", data->peer, data->pl->name);

	data->pl->conn = data;
	player_go(data->pl, SECTOR, ptrlist_entry(univ->sectors, 0));

	conn_loop(data);

	log_printfn("connection", "peer %s disconnected, cleaning up", data->peer);
	conn_cleanexit(data);
	return NULL;
}

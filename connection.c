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

void* connection_worker(void *_data)
{
	struct conndata *data = _data;

	if (data->rbuf[0] != '\0' && cli_run_cmd(data->pl->cli, data->rbuf) < 0)
		conn_send(data, "Unknown command or syntax error: \"%s\"\n", data->rbuf);

	return NULL;
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

void conn_fulfixinit(struct conndata *data)
{
	MALLOC_DIE(data->pl, sizeof(*data->pl));
	player_init(data->pl);

	log_printfn("connection", "peer %s successfully logged in as %s", data->peer, data->pl->name);

	data->pl->conn = data;
	player_go(data->pl, SECTOR, ptrlist_entry(&univ->sectors, 0));

	conn_send(data, "yastg> ");
}

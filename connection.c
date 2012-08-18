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

#include "common.h"
#include "log.h"
#include "connection.h"
#include "server.h"
#include "sarray.h"
#include "ptrlist.h"
#include "id.h"
#include "cli.h"
#include "universe.h"
#include "sector.h"
#include "star.h"
#include "base.h"
#include "planet.h"
#include "player.h"

extern struct universe *univ;

struct conndata* conn_create()
{
	struct conndata *data;
	int r = 1;
	data = malloc(sizeof(*data));
	if (data != NULL) {
		memset(data, 0, sizeof(*data));
		data->id = gen_id();
		data->rbufs = CONN_BUFSIZE;
		if ((data->rbuf = malloc(data->rbufs)) == NULL) {
			log_printfn("connection", "failed allocating receive buffer for connection %zx", data->id);
			conndata_free(data);
			return NULL;
		}
		data->sbufs = CONN_MAXBUFSIZE;
		if ((data->sbuf = malloc(data->sbufs)) == NULL) {
			log_printfn("connection", "failed allocating send buffer for connection %zx", data->id);
			conndata_free(data);
			return NULL;
		}
		if (pipe(data->threadfds) != 0) {
			log_printfn("connection", "failed opening pipe to thread for connection %zx", data->id);
			conndata_free(data);
			return NULL;
		}
	}
	return data;
}

static void conn_signalserver(struct conndata *data, struct signal *msg, char *msgdata)
{
	if (write(data->serverfd, msg, sizeof(*msg)) < 1)
		bug("server signalling fd seems closed when sending signal: error %d (%s)", errno, strerror(errno));
	if (msg->cnt > 0) {
		if (write(data->serverfd, msgdata, msg->cnt) < 1)
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
	log_printfn("connection", "connection %zx is terminating", data->id);  
	struct signal msg = {
		.cnt = sizeof(data->id),
		.type = MSG_RM
	};
	conn_signalserver(data, &msg, (char*)&data->id);
	pthread_exit(0);
}

void conn_send(struct conndata *data, char *format, ...)
{
	va_list ap;
	size_t len, sb = 0;

	va_start(ap, format);
	vsnprintf(data->sbuf, data->sbufs, format, ap);
	va_end(ap);
	len = strlen(data->sbuf);
	if (len == data->sbufs)
		log_printfn("connection", "warning: sending maximum amount allowable (%d bytes) on connection %zx, this probably indicates overflow", data->sbufs, data->id);
	do {
		sb += send(data->peerfd, data->sbuf + sb, len - sb, MSG_NOSIGNAL);
		if (sb < 1) {
			log_printfn("connection", "send error (connection id %zx), terminating connection", data->id);
			conn_cleanexit(data);
		}
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

static void conn_handlesignal(struct conndata *data, struct signal *msg, char *msgdata)
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

static void conn_receivemsg(struct conndata *data, int fd)
{
	struct signal msg;
	char *msgdata;
	read(fd, &msg, sizeof(msg));
	if (msg.cnt > 0) {
		msgdata = alloca(msg.cnt);
		read(fd, msgdata, msg.cnt);
	} else {
		msgdata = NULL;
	}
	conn_handlesignal(data, &msg, msgdata);
}

static void conn_loop(struct conndata *data)
{
	int rb, i;
	char* ptr;

	while (1) {
		rb = 0;

		do {

			conn_send(data, "yastg> ");

			/* FIXME: Doesn't handle CTRL-D for some reason. */
			FD_ZERO(&data->rfds);
			FD_SET(data->peerfd, &data->rfds);
			FD_SET(data->threadfds[0], &data->rfds);
			i = select(MAX(data->peerfd, data->threadfds[0]) + 1, &data->rfds, NULL, NULL, NULL);
			if (i == -1) {
				log_printfn("connection", "select() reported error, terminating connection %zx", data->id);
				conn_cleanexit(data);
			}

			if (FD_ISSET(data->threadfds[0], &data->rfds)) {
				/* We have received a message on the signalling fd */
				do {
					conn_receivemsg(data, data->threadfds[0]);
				} while (data->paused);
			}

			if (FD_ISSET(data->peerfd, &data->rfds)) {

				/* We have received something from the peer */
				rb += recv(data->peerfd, data->rbuf + rb, data->rbufs - rb, 0);
				if (rb < 1) {
					log_printfn("connection", "peer %s disconnected, terminating connection %zx", data->peer, data->id);
					conn_cleanexit(data);
				}
				if ((rb == data->rbufs) && (data->rbuf[rb - 1] != '\n')) {
					if (data->rbufs == CONN_MAXBUFSIZE) {
						log_printfn("connection", "peer sent more data than allowed (%u), connection %zx terminated", CONN_MAXBUFSIZE, data->id);
						conn_cleanexit(data);
					}
					data->rbufs <<= 1;
					if (data->rbufs > CONN_MAXBUFSIZE)
						data->rbufs = CONN_MAXBUFSIZE;
					if ((ptr = realloc(data->rbuf, data->rbufs)) == NULL) {
						data->rbufs >>= 1;
						log_printfn("connection", "unable to increase receive buffer size, connection %zx terminated", data->id);
						conn_cleanexit(data);
					} else {
						data->rbuf = ptr;
					}
				}

			}
		} while ((rb == 0) || (data->rbuf[rb - 1] != '\n'));

		data->rbuf[rb - 1] = '\0';
		chomp(data->rbuf);

		mprintf("debug: received \"%s\" on socket\n", data->rbuf);

		conn_act(data);

	}
}

void* conn_main(void *dataptr)
{
	struct conndata *data = dataptr;
	/* temporary solution */
	/* Create player */
	data->pl = malloc(sizeof(struct player));
	data->pl->name = strdup("Alfred");
	data->pl->conn = data;

	log_printfn("connection", "peer %s successfully logged in as %s", data->peer, data->pl->name);

	player_init(data->pl);

	conn_loop(data);

	log_printfn("connection", "peer %s disconnected, cleaning up", data->peer);
	conn_cleanexit(data);
	return NULL;
}

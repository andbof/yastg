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

#include "defines.h"
#include "log.h"
#include "connection.h"
#include "sarray.h"
#include "ptrarray.h"
#include "id.h"
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
	data->paused = 0;
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
		if (pthread_mutex_init(&data->fd_mutex, NULL) != 0) {
			log_printfn("connection", "failed initializing mutex for connection %zx", data->id);
			conndata_free(data);
			return NULL;
		}
	}
	return data;
}

void conn_signalserver(struct conndata *data, int signal, size_t param)
{
	if ((write(data->serverfd, &signal, sizeof(signal))) < 1)
		bug("server signalling fd seems closed when sending signal to remove me %d, %s", errno, strerror(errno));
	if (write(data->serverfd, &param, sizeof(param)) < 1)
		bug("%s", "server signalling fd seems closed when sending my id");
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
		if (&data->fd_mutex != NULL)
			pthread_mutex_destroy(&data->fd_mutex);
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
	conn_signalserver(data, MSG_RM, data->id);
	pthread_exit(0);
}

void conn_send(struct conndata *data, char *format, ...)
{
	va_list ap;
	size_t len, sb = 0;
	pthread_mutex_lock(&data->fd_mutex);

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
			pthread_mutex_unlock(&data->fd_mutex);
			conn_cleanexit(data); /* FIXME: What happens if another read is waiting to send data? It will try to access a destroyed mutex ... */
		}
	} while (sb < len);

	pthread_mutex_unlock(&data->fd_mutex);
}

void conn_error(struct conndata *data, char *format, ...)
{
	va_list ap;
	conn_send(data, "Oops! An internal error occured.\nYour current state is NOT saved and you are being forcibly disconnected.\nSorry!");
	conn_cleanexit(data);
}

void conn_sendinfo(struct conndata *data)
{
	size_t st;
	char *string;
	struct star *sol;
	struct ptrarray *gurka;
	struct sector *s = data->pl->position, *t;
	if (s == NULL) {
		conn_error(data, "The sector you are in doesn't exist");
		conn_cleanexit(data);
	}
	conn_send(data, "You are in sector %s (coordinates %ldx%ld), habitability %d\n", s->name, s->x, s->y, s->hab);
	conn_send(data, "Snow line at %lu Gm\n", s->snowline);
	conn_send(data, "Habitable zone is from %lu to %lu Gm\n", s->hablow, s->habhigh);
	for (st = 0; st < s->stars->elements; st++) {
		sol = ptrarray_get(s->stars, st);
		conn_send(data, "%s: Class %c %s\n", sol->name, stellar_cls[sol->cls], stellar_lum[sol->lum]);
		string = hundreths(sol->lumval);
		conn_send(data, "  Surface temperature: %dK, habitability modifier: %d, luminosity: %s \n", sol->temp, sol->hab, string);
		free(string);
	}
	conn_send(data, "This sector has hyperspace links to\n");
	for (st = 0; st < s->links->elements; st++) {
		conn_send(data, "  %s\n", ((struct sector*)ptrarray_get(s->links, st))->name);
	}
	conn_send(data, "Sectors within 50 lys are:\n");
	/* FIXME: getneighbours() is awful */
	gurka = getneighbours(s, 50);
	for (st = 0; st < gurka->elements; st++) {
		t = ptrarray_get(gurka, st);
		conn_send(data, "  %s at %lu ly\n", t->name, sector_distance(s, t));
	}
	ptrarray_free(gurka);
	if (s->owner != NULL) {
		conn_send(data, "This sector is owned by civ %zx\n", s->name);
	} else {
		conn_send(data, "This sector is not part of any civilization\n");
	}
}

void conn_act(struct conndata *data)
{
	struct sector *s;
	if (!strcmp(data->rbuf, "help")) {
		conn_send(data, "go <sector>	Move to sector <name>");
	} else if (!strcmp(data->rbuf, "quit")) {
		conn_send(data, "Bye!\n");
		conn_cleanexit(data);
	} else if (!strncmp(data->rbuf, "go ", 3)) {
		if (strlen(data->rbuf) > 3) {
			s = getsectorbyname(univ, data->rbuf+3);
			if (s != NULL) {
				conn_send(data, "Entering %s\n", s->name);
				data->pl->position = s;
				conn_sendinfo(data);
			} else {
				conn_send(data, "Sector not found.\n");
			}
		} else {
			conn_send(data, ERR_SYNTAX);
		}
	} else {
		conn_send(data, "Sorry, I don't understand.\n");
	}
}

void conn_handlesignal(struct conndata *data, enum msg signal)
{
	char *str;
	switch (signal) {
		case MSG_TERM:
			conn_send(data, "\nServer is shutting down, you are being disconnected.\n");
			conn_cleanexit(data);
			break;
		case MSG_PAUSE:
			conn_send(data, "\nYou have been paused by God. This might mean the whole universe is currently on hold\n");
			conn_send(data, "or just you. Anything you enter at the prompt will queue up until you are resumed.\n");
			data->paused = 1;
			break;
		case MSG_CONT:
			conn_send(data, "\nYou have been resumed, feel free to play away!\n");
			data->paused = 0;
			break;
		case MSG_WALL:
			conn_send(data, "\nMessage to all connected users:\n");
			read(data->threadfds[0], &str, sizeof(char*));
			conn_send(data, str);
			conn_send(data, "\nEnd of message.\n");
			break;
		default:
			log_printfn("connection", "received unknown signal: %d", signal);
	}
}

void conn_loop(struct conndata *data)
{
	int rb, i;
	char* ptr;

	conn_sendinfo(data);

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
					read(data->threadfds[0], &i, sizeof(i));
					conn_handlesignal(data, i);
				} while (data->paused);
			}

			if (FD_ISSET(data->peerfd, &data->rfds)) {

				/* We have received something from the peer
				   FIXME: If peer sends something with a line break, we should interpret it as separate commands */
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

		/* Truncate string at EOL (we might have \13\10 or \10) */
		if ((rb > 1) && (data->rbuf[rb - 2] == 13))
			data->rbuf[rb - 2] = '\0';
		else
			data->rbuf[rb - 1] = '\0';

		mprintf("received \"%s\" on socket\n", data->rbuf);

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
	data->pl->position = ptrarray_get(univ->sectors, 0);

	log_printfn("connection", "peer %s successfully logged in as %s", data->peer, data->pl->name);
	conn_loop(data);
	log_printfn("connection", "peer %s disconnected, cleaning up", data->peer);
	conn_cleanexit(data);
	return NULL;
}

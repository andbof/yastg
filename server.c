#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>

#include "common.h"
#include "log.h"
#include "server.h"
#include "sarray.h"
#include "connection.h"
#include "id.h"

#define PORT "2049"
#define BACKLOG 16	/* Size of pending connections queue */

static struct sarray *std;
static pthread_t srvthread;
static fd_set fdset;
static int sockfd, signfdw, signfdr, maxfd;

static void server_cleanexit()
{
	int i, *j;
	struct signal msg = {
		.cnt = 0,
		.type = MSG_TERM
	};
	struct conndata *cd;
	log_printfn("server", "sending terminate to all player threads");
	for (i = 0; i < std->elements; i++) {
		cd = sarray_getbypos(std, i);
		if (write(cd->threadfds[1], &msg, sizeof(msg)) < 1)
			bug("thread %zx's signalling fd seems closed", cd->id);
	}
	log_printfn("server", "waiting for all player threads to terminate");
	for (i = 0; i < std->elements; i++) {
		cd = sarray_getbypos(std, i);
		/* FIXME : This is a race condition if cd is free'd between the if statement and pthread_join */
		if (cd != NULL)
			pthread_join(cd->thread, NULL);
	}
	log_printfn("server", "server shutting down");
	sarray_free(std);
	free(std);
	close(sockfd);
	pthread_exit(0);
}

static void server_write_msg(int fd, struct signal *msg, char *msgdata)
{
	/* FIXME: This should be handled more gracefully */
	if (write(fd, msg, sizeof(*msg)) < 1)
		bug("%s", "a thread's signalling fd seems closed");
	if (msg->cnt > 0)
		if (write(fd, msgdata, msg->cnt) < 1)
			bug("%s", "a thread's signalling fd seems closed");
}

static void server_signallthreads(struct signal *msg, char *msgdata)
{
	struct conndata *cd;
	/* FIXME: This isn't thread safe, it assumes that std is not modified during the whole sending process */
	for (int i = 0; i < std->elements; i++) {
		cd = sarray_getbypos(std, i);
		server_write_msg(cd->threadfds[1], msg, msgdata);
	}
}

static void server_handlesignal(struct signal *msg, char *data)
{
	struct conndata *cd;
	int i, j;
	size_t st;
	log_printfn("server", "received signal %d", msg->type);
	switch (msg->type) {
	case MSG_TERM:
		server_cleanexit();
		break;
	case MSG_RM:
		st = *(size_t*)data;
		log_printfn("server", "thread %zx is terminating, cleaning up", st);
		cd = sarray_getbyid(std, &st);
		if (cd != NULL) {
			pthread_join(cd->thread, NULL);
			sarray_freebyid(std, &st);
		} else {
			bug("unknown thread id %zx is terminating", st);
		}
		break;
	case MSG_WALL:
		log_printfn("server", "walling all users: %s", data);
		server_signallthreads(msg, data);
		break;
	case MSG_PAUSE:
		log_printfn("server", "pausing the entire universe");
		server_signallthreads(msg, NULL);
		break;
	case MSG_CONT:
		log_printfn("server", "universe continuing");
		server_signallthreads(msg, NULL);
		break;
	default:
		log_printfn("server", "unknown message received: %d", msg->type);
	}
}

static void server_preparesocket(struct addrinfo **servinfo)
{
	int i;
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		/* Don't care if we bind to IPv4 or IPv6 sockets */
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((i = getaddrinfo(NULL, PORT, &hints, servinfo)) != 0)
		die("getaddrinfo: %s\n", gai_strerror(i));
}

/* get sockaddr, IPv4 or IPv6: */
static void* server_get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	} else {
		return &(((struct sockaddr_in6*)sa)->sin6_addr);
	}
}

static int server_setupsocket()
{
	struct addrinfo *servinfo, *p;
	server_preparesocket(&servinfo);
	int fd;
	int yes = 1;

	/* We loop trough each ai structure to get the first one that actually works */
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			mprintf("%s", "socket error");
			continue;
		}
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == 1)
			die("%s", "setsockopt error");
		if (bind(fd, p->ai_addr, p->ai_addrlen) == -1) {
			close(fd);
			mprintf("%s", "bind error");
			continue;
		}
		break;
	}
	if (p == NULL)
		die("%s", "server failed to bind");
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
		die("failed setting server socket to non-blocking, error %s", strerror(errno));
	if (listen(fd, BACKLOG) == -1)
		die("%s", "listen error");

	/* The servinfo structure was only used to get an ai struct, so we can free it now */
	freeaddrinfo(servinfo);

	return fd;
}

static char* getpeer(struct sockaddr_storage sock)
{
	struct sockaddr_in *s4 = (struct sockaddr_in*)&sock;
	struct sockaddr_in6 *s6 = (struct sockaddr_in6*)&sock;
	char *result = malloc(INET6_ADDRSTRLEN + 6);
	if (sock.ss_family == AF_INET) {
		inet_ntop(AF_INET, &s4->sin_addr, result, INET6_ADDRSTRLEN + 6);
		sprintf(result + strlen(result), ":%d", ntohs(s4->sin_port));
	} else {
		inet_ntop(AF_INET6, &s6->sin6_addr, result, INET6_ADDRSTRLEN + 6);
		sprintf(result + strlen(result), ":%d", ntohs(s6->sin6_port));
	}
	return result;
}

static void server_receivemsg(int fd)
{
	struct signal msg;
	char *data;
	int r = read(fd, &msg, sizeof(msg));
	if (msg.cnt > 0) {
		data = alloca(msg.cnt);
		read(fd, data, msg.cnt);	/* FIXME: Validate the number of bytes */
	} else {
		data = NULL;
	}
	server_handlesignal(&msg, data);
}

void* server_main(void* p)
{
	int i, j;
	unsigned long l;
	size_t st;
	struct sockaddr_storage peer_addr;
	socklen_t sin_size = sizeof(peer_addr);
	std = sarray_init(sizeof(struct conndata), 0, SARRAY_ENFORCE_UNIQUE, &conndata_free, &sort_id);
	struct conndata *cd;

	srvthread = pthread_self();
	signfdr = *(int*)p;
	signfdw = *((int*)p + 1);
	sockfd = server_setupsocket();
	maxfd = MAX(sockfd, signfdr);
	maxfd++;

	log_printfn("server", "server is up waiting for connections on port %s", PORT);

	while (1) {
		FD_ZERO(&fdset);
		FD_SET(sockfd, &fdset);
		FD_SET(signfdr, &fdset);
		i = select(maxfd, &fdset, NULL, NULL, NULL);
		if (i == -1)
			die("select() failed in server, error %s", strerror(errno));
		if (FD_ISSET(signfdr, &fdset)) {
			/* We have received a message on the signalling file handle */
			server_receivemsg(signfdr);
		} else {
			/* We have received a new connection, accept it */
			cd = conn_create();
			if (cd == NULL) {
				log_printfn("server", "failed creating connection data structure");
				continue;
			}
			st = cd->id;
			sarray_add(std, cd);
			free(cd);
			cd = sarray_getbyid(std, &st);
			cd->peerfd = accept(sockfd, (struct sockaddr*)&peer_addr, &sin_size);
			cd->serverfd = signfdw;
			if (cd->peerfd == -1) {
				if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
					log_printfn("server", "a peer disconnected before connection could be properly accepted");
					conndata_free(cd);
					free(cd);
				} else {
					log_printfn("server", "could not accept socket connection, error %s", strerror(errno));
					conndata_free(cd);
					free(cd);
				}
			} else {
				/*	inet_ntop(peer_addr.ss_family, server_get_in_addr((struct sockaddr*)&peer_addr), std[tnum].peer, sizeof(std[tnum].peer)); */
				socklen_t len = sizeof(cd->sock);
				getpeername(cd->peerfd, (struct sockaddr*)&cd->sock, &len);
				cd->peer = getpeer(cd->sock);
				log_printfn("server", "new connection from %s, assigning id %zx", cd->peer, cd->id);
				if ((i = pthread_create(&cd->thread, NULL, conn_main, cd))) {
					log_printfn("failed creating thread to handle connection from %s: %i", cd->peer, i);
				}
			}
		}
	}

}

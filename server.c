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

#include "defines.h"
#include "log.h"
#include "server.h"
#include "sarray.h"
#include "connection.h"
#include "id.h"

#define PORT "2049"
#define BACKLOG 16	// Size of pending connections queue

static struct sarray *std;
static pthread_t srvthread;
static fd_set fdset;
static int sockfd, signfdw, signfdr, maxfd;

void server_cleanexit() {
	int i, *j;
	enum msg m = MSG_TERM;
	struct conndata *cd;
	log_printfn("server", "sending terminate to all player threads");
	for (i = 0; i < std->elements; i++) {
		cd = sarray_getbypos(std, i);
		if (write(cd->threadfds[1], &m, sizeof(m)) < 1)
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

void server_signthread(int fd, void *msg, size_t size) {
	// FIXME: This should be handled more gracefully
	if (write(fd, msg, size) < 1)
		bug("%s", "a thread's signalling fd seems closed");
}

void server_signallthreads(void *msg, size_t size) {
	int i;
	struct conndata *cd;
	// FIXME: This isn't thread safe, it assumes that std is not modified during the whole sending process
	for (i = 0; i < std->elements; i++) {
		cd = sarray_getbypos(std, i);
		server_signthread(cd->threadfds[1], msg, size);
	}
}

void server_handlesignal(enum msg signal, size_t param) {
	struct conndata *cd;
	int i, j;
	log_printfn("server", "received signal %d", signal);
	switch (signal) {
		case MSG_TERM:
			server_cleanexit();
			break;
		case MSG_RM:
			log_printfn("server", "thread %zx is terminating, cleaning up", param);
			cd = sarray_getbyid(std, &param);
			if (cd != NULL) {
				pthread_join(cd->thread, NULL);
				sarray_freebyid(std, &param);
			} else {
				bug("unknown thread id %zx is terminating", param);
			}
			break;
		case MSG_WALL:
			// FIXME: This assumes that the string at &param is valid for as long as there are threads to
			// send it to; it should be duplicated.
			log_printfn("server", "walling all users");
			server_signallthreads(&signal, sizeof(int));
			server_signallthreads(&param, sizeof(void*));
			break;
		case MSG_PAUSE:
			log_printfn("server", "pausing the entire universe");
			server_signallthreads(&signal, sizeof(int));
			break;
		case MSG_CONT:
			log_printfn("server", "universe continuing");
			server_signallthreads(&signal, sizeof(int));
			break;
		default:
			log_printfn("server", "unknown message received: %d", signal);
	}
}

void server_preparesocket(struct addrinfo **servinfo) {
	int i;
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// Don't care if we bind to IPv4 or IPv6 sockets
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((i = getaddrinfo(NULL, PORT, &hints, servinfo)) != 0) {
		die("getaddrinfo: %s\n", gai_strerror(i));
	}
}

// get sockaddr, IPv4 or IPv6:
void* server_get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	} else {
		return &(((struct sockaddr_in6*)sa)->sin6_addr);
	}
}

int server_setupsocket() {
	struct addrinfo *servinfo, *p;
	server_preparesocket(&servinfo);
	int fd;
	int yes = 1;

	// We loop trough each ai structure to get the first one that actually works
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
		die("%s", "server failed to bind"); // None of them worked
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
		die("failed setting server socket to non-blocking, error %s", strerror(errno));
	if (listen(fd, BACKLOG) == -1)
		die("%s", "listen error");

	// The servinfo structure was only used to get an ai struct, so we can free it now
	freeaddrinfo(servinfo);

	return fd;
}

char* getpeer(struct sockaddr_storage sock) {
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

void* server_main(void* p) {
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
			// We have received a message on the signalling file handle
			read(signfdr, &i, sizeof(i));
			read(signfdr, &st, sizeof(st));
			server_handlesignal(i, st);
		} else {
			// We have received a new connection, accept it
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
				//	inet_ntop(peer_addr.ss_family, server_get_in_addr((struct sockaddr*)&peer_addr), std[tnum].peer, sizeof(std[tnum].peer));
				st = sizeof(cd->sock);
				getpeername(cd->peerfd, (struct sockaddr*)&cd->sock, &st);
				cd->peer = getpeer(cd->sock);
				log_printfn("server", "new connection from %s, assigning id %zx", cd->peer, cd->id);
				if ((i = pthread_create(&cd->thread, NULL, conn_main, cd))) {
					log_printfn("failed creating thread to handle connection from %s: %i", cd->peer, i);
				}
			}
		}
	}

}

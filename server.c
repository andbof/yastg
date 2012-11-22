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
#include <ev.h>

#include "common.h"
#include "log.h"
#include "server.h"
#include "connection.h"

#define PORT "2049"
#define BACKLOG 16	/* Size of pending connections queue */

static fd_set fdset;
static int sockfd, signfdw, signfdr;

static struct conndata connlist;
pthread_rwlock_t connlist_lock;

static void server_cleanexit()
{
	int i, *j;
	struct signal msg = {
		.cnt = 0,
		.type = MSG_TERM
	};
	struct conndata *cd;

	pthread_rwlock_wrlock(&connlist_lock);

	log_printfn("server", "sending terminate to all player threads");
	list_for_each_entry(cd, &connlist.list, list)
		if (write(cd->threadfds[1], &msg, sizeof(msg)) < 1)
			bug("thread %x's signalling fd seems closed", cd->id);

	log_printfn("server", "waiting for all player threads to terminate");
	list_for_each_entry(cd, &connlist.list, list)
		pthread_join(cd->thread, NULL);

	log_printfn("server", "server shutting down");

	struct conndata *tmp;
	list_for_each_entry_safe(cd, tmp, &connlist.list, list) {
		list_del(&cd->list);
		conndata_free(cd);
		free(cd);
	}

	pthread_rwlock_destroy(&connlist_lock);

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
	pthread_rwlock_rdlock(&connlist_lock);
	list_for_each_entry(cd, &connlist.list, list)
		server_write_msg(cd->threadfds[1], msg, msgdata);
	pthread_rwlock_unlock(&connlist_lock);
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
		pthread_rwlock_wrlock(&connlist_lock);
		list_for_each_entry(cd, &connlist.list, list) {
			if (cd->id == *(uint32_t*)data) {
				log_printfn("server", "thread %x is terminating, cleaning up", cd->id);
				pthread_join(cd->thread, NULL);
				list_del(&cd->list);
				conndata_free(cd);
				free(cd);
				cd = NULL;
				break;
			}
		}
		pthread_rwlock_unlock(&connlist_lock);
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

static void server_receivemsg()
{
	struct signal msg;
	char *data;
	int r = read(signfdr, &msg, sizeof(msg));
	if (msg.cnt > 0) {
		data = alloca(msg.cnt);
		read(signfdr, data, msg.cnt);	/* FIXME: Validate the number of bytes */
	} else {
		data = NULL;
	}

	server_handlesignal(&msg, data);
}

static void server_msg_cb(struct ev_loop *loop, ev_io *w, int revents)
{
	return server_receivemsg();
}

int server_accept_connection()
{
	int i;
	struct conndata *cd;
	size_t st;
	struct sockaddr_storage peer_addr;
	socklen_t sin_size = sizeof(peer_addr);

	cd = conn_create();
	if (cd == NULL) {
		log_printfn("server", "failed creating connection data structure");
		return -1;
	}

	cd->peerfd = accept(sockfd, (struct sockaddr*)&peer_addr, &sin_size);
	cd->serverfd = signfdw;
	if (cd->peerfd == -1) {
		if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
			log_printfn("server", "a peer disconnected before connection could be properly accepted");
			goto err_free;
		} else {
			log_printfn("server", "could not accept socket connection, error %s", strerror(errno));
			goto err_free;
		}
	} else {
		/*	inet_ntop(peer_addr.ss_family, server_get_in_addr((struct sockaddr*)&peer_addr), std[tnum].peer, sizeof(std[tnum].peer)); */
		socklen_t len = sizeof(cd->sock);
		getpeername(cd->peerfd, (struct sockaddr*)&cd->sock, &len);
		cd->peer = getpeer(cd->sock);
		log_printfn("server", "new connection %x from %s", cd->id, cd->peer);

		pthread_rwlock_wrlock(&connlist_lock);
		list_add_tail(&cd->list, &connlist.list);	/* FIXME: thread should be created, sleep, cd should be added to connlist, THEN thread started. Otherwise we have a race here before the thread structure is initialized if something traverses the list */
		pthread_rwlock_unlock(&connlist_lock);

		if ((i = pthread_create(&cd->thread, NULL, conn_main, cd)))
			log_printfn("failed creating thread to handle connection from %s: %i", cd->peer, i);
	}

	return 0;

err_free:
	conndata_free(cd);
	free(cd);
	return -1;
}

static void server_accept_cb(struct ev_loop *loop, ev_io *w, int revents)
{
	server_accept_connection();
}

void* server_main(void* p)
{
	int i;
	ev_io msg_watcher, accept_watcher;
	struct ev_loop *loop = EV_DEFAULT;

	INIT_LIST_HEAD(&connlist.list);
	pthread_rwlock_init(&connlist_lock, NULL);

	signfdr = *(int*)p;
	signfdw = *((int*)p + 1);
	sockfd = server_setupsocket();

	ev_io_init(&msg_watcher, server_msg_cb, signfdr, EV_READ);
	ev_io_init(&accept_watcher, server_accept_cb, sockfd, EV_READ);

	ev_io_start(loop, &msg_watcher);
	ev_io_start(loop, &accept_watcher);

	log_printfn("server", "server is up waiting for connections on port %s", PORT);

	ev_run(loop, 0);

	ev_io_stop(loop, &msg_watcher);
	ev_io_stop(loop, &accept_watcher);

	/*
	 * We'd like to free all resources allocated by libev here, but
	 * unfortunately there doesn't seem to be any good way of doing so.
	 */

	return NULL;
}

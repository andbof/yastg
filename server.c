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
#include <semaphore.h>
#include <ev.h>

#include "common.h"
#include "log.h"
#include "server.h"
#include "connection.h"

static int sockfd, signfdw, signfdr;

static struct conn_data conn_data;
static struct list_head conn_list;
pthread_rwlock_t conn_list_lock;

struct socket_list {
	int fd;
	struct list_head list;
};

struct watcher_list {
	ev_io watcher;
	int fd;
	struct list_head list;
};

static void disconnect_peers()
{
	struct connection *cd;

	list_for_each_entry(cd, &conn_list, list) {
		conn_send(cd, "Server is shutting down, you are being disconnected.\n");
		conn_cleanexit(cd);
	}
}

static void server_handlesignal(struct ev_loop *loop, struct signal *msg, char *data)
{
	struct connection *cd, *p;
	int i, j;
	size_t st;
	log_printfn("server", "received signal %d", msg->type);
	switch (msg->type) {
	case MSG_TERM:
		/* This will break all event loops, especially the main server loop in server_main() */
		ev_unloop(EV_A_ EVUNLOOP_ALL);
		break;
	case MSG_RM:
		pthread_rwlock_wrlock(&conn_list_lock);
		list_for_each_entry_safe(cd, p, &conn_list, list) {
			if (cd->id == *(uint32_t*)data) {
				log_printfn("server", "thread %x is terminating, cleaning up", cd->id);
				ev_io_stop(loop, &cd->watcher);
				list_del(&cd->list);
				connection_free(cd);
				free(cd);
				cd = NULL;
				break;
			}
		}
		pthread_rwlock_unlock(&conn_list_lock);
		break;
	case MSG_WALL:
		log_printfn("server", "walling all users: %s", data);
		pthread_rwlock_rdlock(&conn_list_lock);
		list_for_each_entry(cd, &conn_list, list)
			conn_send(cd, "\nMessage to all connected users:\n"
					"%s"
					"\nEnd of message.\n", data);
		pthread_rwlock_unlock(&conn_list_lock);
		break;
	case MSG_PAUSE:
		log_printfn("server", "pausing the entire universe");
		pthread_rwlock_rdlock(&conn_list_lock);
		list_for_each_entry(cd, &conn_list, list) {
			ev_io_stop(loop, &cd->watcher);
			conn_send(cd, "\nYou have been paused by God. This might mean the whole universe is currently on hold\n"
					"or just you. Anything you enter at the prompt will queue up until you are resumed.\n");
		}
		pthread_rwlock_unlock(&conn_list_lock);
		break;
	case MSG_CONT:
		/* FIXME: CONT */
		log_printfn("server", "universe continuing");
		pthread_rwlock_rdlock(&conn_list_lock);
		list_for_each_entry(cd, &conn_list, list) {
			ev_io_start(loop, &cd->watcher);
			conn_send(cd, "\nYou have been resumed, feel free to play away!\n");
		}
		pthread_rwlock_unlock(&conn_list_lock);
		break;
	default:
		log_printfn("server", "unknown message received: %d", msg->type);
	}
}

#define SERVER_PORT "2049"
static void server_preparesocket(struct addrinfo **servinfo)
{
	int i;
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		/* Don't care if we bind to IPv4 or IPv6 sockets */
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((i = getaddrinfo(NULL, SERVER_PORT, &hints, servinfo)) != 0)
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

#define SERVER_MAX_PENDING_CONNECTIONS 16
static int setupsocket(struct addrinfo *p)
{
	int fd;

	fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
	if (fd < 0)
		return -1;

	int yes = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)))
		goto err;

	/* This will fail on newer BSDs since they turned off IPv4->IPv6 mapping.
	 * That's perfectly OK but we can't check the exit status. */
	if (p->ai_family == AF_INET6)
		setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &yes, sizeof(yes));

	if (bind(fd, p->ai_addr, p->ai_addrlen) < 0)
		goto err;

	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
		goto err;

	if (listen(fd, SERVER_MAX_PENDING_CONNECTIONS) < 0)
		goto err;

	return fd;

err:
	close(fd);
	return -1;
}

static void close_and_free_sockets(struct list_head *sockets)
{
	struct socket_list *s, *_s;

	list_for_each_entry(s, sockets, list)
		if (s->fd > 0)
			close(s->fd);

	list_for_each_entry_safe(s, _s, sockets, list) {
		list_del(&s->list);
		free(s);
	}
}

static int initialize_server_sockets(struct list_head *sockets)
{
	struct addrinfo *servinfo, *p;
	server_preparesocket(&servinfo);
	int fd;
	int yes = 1;
	struct socket_list *s;

	/* We want to bind to all valid combinations returned by getaddrinfo()
	 * to make sure we support IPv4 and IPv6 */
	for (p = servinfo; p != NULL; p = p->ai_next) {
		fd = setupsocket(p);
		if (fd < 0)
			continue;

		s = malloc(sizeof(*s));
		if (!s)
			goto err;

		s->fd = fd;
		list_add(&s->list, sockets);
	}

	freeaddrinfo(servinfo);

	return 0;

err:
	close_and_free_sockets(sockets);

	return -1;
}

static char* getpeer(const struct sockaddr_storage sock)
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

static void server_msg_cb(struct ev_loop * const loop, ev_io * const w, const int revents)
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

	server_handlesignal(loop, &msg, data);
}

static void receive_peer_data(struct connection *data)
{
	void *ptr;

	/* FIXME: Check if a worker is working on the data already, if so drop new data */

	data->rbufi += recv(data->peerfd, data->rbuf + data->rbufi, data->rbufs - data->rbufi, 0);
	if (data->rbufi< 1) {
		log_printfn("server", "peer %s disconnected, terminating connection %x", data->peer, data->id);
		conn_cleanexit(data);
	}
	if ((data->rbufi == data->rbufs) && (data->rbuf[data->rbufi - 1] != '\n')) {
		if (data->rbufs == CONN_MAXBUFSIZE) {
			log_printfn("server", "peer sent more data than allowed (%u), connection %x terminated", CONN_MAXBUFSIZE, data->id);
			conn_cleanexit(data);
		}
		data->rbufs <<= 1;
		if (data->rbufs > CONN_MAXBUFSIZE)
			data->rbufs = CONN_MAXBUFSIZE;
		if ((ptr = realloc(data->rbuf, data->rbufs)) == NULL) {
			data->rbufs >>= 1;
			log_printfn("server", "unable to increase receive buffer size, connection %x terminated", data->id);
			conn_cleanexit(data);
		} else {
			data->rbuf = ptr;
		}
	}

	if (data->rbufi != 0 && data->rbuf[data->rbufi - 1] == '\n') {
		data->rbuf[data->rbufi - 1] = '\0';
		chomp(data->rbuf);

		mprintf("debug: received \"%s\" on socket\n", data->rbuf);
		
		conn_do_work(&conn_data, data);

		data->rbufi = 0;
	}
}

static void got_new_peer_data(struct ev_loop * const loop, ev_io * const w, const int revents)
{
	struct connection *data = (struct connection*)w->data;
	receive_peer_data(data);
}

int server_accept_connection(struct ev_loop * const loop, int fd)
{
	int i;
	struct connection *cd;
	size_t st;
	struct sockaddr_storage peer_addr;
	socklen_t sin_size = sizeof(peer_addr);

	cd = conn_create();
	if (cd == NULL) {
		log_printfn("server", "failed creating connection data structure");
		return -1;
	}

	cd->peerfd = accept(fd, (struct sockaddr*)&peer_addr, &sin_size);
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

		pthread_rwlock_wrlock(&conn_list_lock);
		list_add_tail(&cd->list, &conn_list);
		ev_io_init(&cd->watcher, got_new_peer_data, cd->peerfd, EV_READ);
		cd->watcher.data = cd;
		pthread_rwlock_unlock(&conn_list_lock);

		ev_io_start(loop, &cd->watcher);

		log_printfn("server", "serving new connection %x", cd->id);
		conn_fulfixinit(cd);
	}

	return 0;

err_free:
	connection_free(cd);
	free(cd);
	return -1;
}

static void server_accept_cb(struct ev_loop * const loop, ev_io * const w, const int revents)
{
	struct socket_list *s = w->data;
	server_accept_connection(loop, s->fd);
}

static void stop_and_free_server_watchers(struct list_head *watchers, struct ev_loop *loop)
{
	struct watcher_list *w, *_w;

	list_for_each_entry(w, watchers, list)
		ev_io_stop(loop, &w->watcher);

	list_for_each_entry_safe(w, _w, watchers, list) {
		list_del(&w->list);
		free(w);
	}
}

static int start_server_watchers(struct list_head *watchers, struct ev_loop *loop, struct list_head *sockets)
{
	struct watcher_list *w;
	struct socket_list *s;

	list_for_each_entry(s, sockets, list) {
		w = malloc(sizeof(*w));
		if (!w)
			goto err;

		ev_io_init(&w->watcher, server_accept_cb, s->fd, EV_READ);
		w->watcher.data = s;
		list_add(&w->list, watchers);
	}

	list_for_each_entry(w, watchers, list)
		ev_io_start(loop, &w->watcher);

	return 0;

err:
	stop_and_free_server_watchers(watchers, loop);

	return -1;
}

void* server_main(void* p)
{
	int i;
	ev_io msg_watcher, accept_watcher;
	struct ev_loop *loop = EV_DEFAULT;
	LIST_HEAD(sockets);
	LIST_HEAD(watchers);

	INIT_LIST_HEAD(&conn_list);
	pthread_rwlock_init(&conn_list_lock, NULL);

	signfdr = *(int*)p;
	signfdw = *((int*)p + 1);

	if (conn_init(&conn_data))
		die("%s", "failed initializing connection data structures");

	initialize_server_sockets(&sockets);
	if (list_empty(&sockets))
		die("%s", "server failed to bind");

	start_server_watchers(&watchers, loop, &sockets);
	if (list_empty(&watchers))
		die("%s", "server failed to create watchers");

	ev_io_init(&msg_watcher, server_msg_cb, signfdr, EV_READ);

	ev_io_start(loop, &msg_watcher);

	log_printfn("server", "server is up waiting for connections on port %s", SERVER_PORT);

	ev_run(loop, 0);

	ev_io_stop(loop, &msg_watcher);

	disconnect_peers();
	stop_and_free_server_watchers(&watchers, loop);
	close_and_free_sockets(&sockets);

	/*
	 * We'd like to free all resources allocated by libev here, but
	 * unfortunately there doesn't seem to be any good way of doing so.
	 */

	conn_shutdown(&conn_data);
	conn_destroy(&conn_data);

	struct connection *cd, *tmp;
	list_for_each_entry_safe(cd, tmp, &conn_list, list) {
		list_del(&cd->list);
		connection_free(cd);
		free(cd);
	}

	close(sockfd);

	return NULL;
}

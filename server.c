#include <config.h>
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

#include "port_update.h"
#include "common.h"
#include "log.h"
#include "server.h"
#include "connection.h"

static int sockfd, signfdw, signfdr;
static struct ev_loop *loop;

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

static void disconnect_peer(struct ev_loop *loop, struct connection *conn)
{
	struct connection *c, *_c;
	log_printfn(LOG_SERVER, "now terminating connection %x", conn->id);

	ev_io_stop(loop, &conn->data_watcher);
	ev_async_stop(loop, &conn->kill_watcher);

	/*
	 * Remember: The locking in this function needs to be synchronized
	 * with the locking in connection_worker() to avoid nasty races in case
	 * a connection is terminated when connection_worker() is just starting.
	 */

	pthread_mutex_lock(&conn_data.workers_lock);
	list_for_each_entry_safe(c, _c, &conn_data.work_items, work) {
		if (c == conn)
			list_del_init(&c->work);
	}
	pthread_mutex_unlock(&conn_data.workers_lock);

	while (conn->worker);
	pthread_mutex_lock(&conn->worker_lock);

	pthread_rwlock_wrlock(&conn_list_lock);
	list_del(&conn->list);
	pthread_rwlock_unlock(&conn_list_lock);

	log_printfn(LOG_SERVER, "connection %x successfully terminated", conn->id);
	connection_free(conn);
	free(conn);
}

void server_disconnect_cb(struct ev_loop *loop, struct ev_async *w, int revents)
{
	struct connection *conn = w->data;
	disconnect_peer(loop, conn);
}

void server_disconnect_nicely(struct connection *conn)
{
	if (conn->terminate)
		return;

	log_printfn(LOG_SERVER, "asking nicely to terminate connection %x", conn->id);
	conn->terminate = 1;
	ev_async_send(loop, &conn->kill_watcher);
}

static void disconnect_peers(struct ev_loop *loop)
{
	struct connection *cd, *_cd;

	list_for_each_entry_safe(cd, _cd, &conn_list, list) {
		conn_send(cd, "Server is shutting down, you are being disconnected.\n");
		disconnect_peer(loop, cd);
	}
}

static void server_handlesignal(struct ev_loop *loop, struct signal *msg, char *data)
{
	struct connection *cd;
	log_printfn(LOG_SERVER, "received signal %d", msg->type);
	switch (msg->type) {
	case MSG_TERM:
		/* This will break all event loops, especially the main server loop in server_main() */
		ev_unloop(EV_A_ EVUNLOOP_ALL);
		break;
	case MSG_WALL:
		log_printfn(LOG_SERVER, "walling all users: %s", data);
		pthread_rwlock_rdlock(&conn_list_lock);
		list_for_each_entry(cd, &conn_list, list)
			conn_send(cd, "\nMessage to all connected users:\n"
					"%s"
					"\nEnd of message.\n", data);
		pthread_rwlock_unlock(&conn_list_lock);
		break;
	case MSG_PAUSE:
		log_printfn(LOG_SERVER, "pausing the entire universe");
		pthread_rwlock_rdlock(&conn_list_lock);
		list_for_each_entry(cd, &conn_list, list) {
			ev_io_stop(loop, &cd->data_watcher);
			conn_send(cd, "\nYou have been paused by God. This might mean the whole universe is currently on hold\n"
					"or just you. Anything you enter at the prompt will queue up until you are resumed.\n");
		}
		pthread_rwlock_unlock(&conn_list_lock);
		break;
	case MSG_CONT:
		/* FIXME: CONT */
		log_printfn(LOG_SERVER, "universe continuing");
		pthread_rwlock_rdlock(&conn_list_lock);
		list_for_each_entry(cd, &conn_list, list) {
			ev_io_start(loop, &cd->data_watcher);
			conn_send(cd, "\nYou have been resumed, feel free to play away!\n");
		}
		pthread_rwlock_unlock(&conn_list_lock);
		break;
	default:
		log_printfn(LOG_SERVER, "unknown message received: %d", msg->type);
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

static char* pretty_print_peer(char * const out, const size_t len, const struct sockaddr_storage sock)
{
	struct sockaddr_in *s4 = (struct sockaddr_in*)&sock;
	struct sockaddr_in6 *s6 = (struct sockaddr_in6*)&sock;
	if (len < INET6_ADDRSTRLEN + 7)
		return NULL;

	if (sock.ss_family == AF_INET) {
		inet_ntop(AF_INET, &s4->sin_addr, out, INET6_ADDRSTRLEN + 6);
		sprintf(out + strlen(out), ":%d", ntohs(s4->sin_port));
	} else {
		inet_ntop(AF_INET6, &s6->sin6_addr, out, INET6_ADDRSTRLEN + 6);
		sprintf(out + strlen(out), ":%d", ntohs(s6->sin6_port));
	}
	return out;
}

static void server_msg_cb(struct ev_loop * const loop, ev_io * const w, const int revents)
{
	struct signal msg;
	char *data;

	read(signfdr, &msg, sizeof(msg));
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

	data->rbufi += recv(data->peerfd, data->rbuf + data->rbufi, data->rbufs - data->rbufi, 0);
	if (data->rbufi< 1) {
		log_printfn(LOG_SERVER, "peer %s disconnected, terminating connection %x", data->peer, data->id);
		server_disconnect_nicely(data);
	}
	if ((data->rbufi == data->rbufs) && (data->rbuf[data->rbufi - 1] != '\n')) {
		if (data->rbufs == CONN_MAXBUFSIZE) {
			log_printfn(LOG_SERVER, "peer sent more data than allowed (%u), connection %x terminated", CONN_MAXBUFSIZE, data->id);
			server_disconnect_nicely(data);
		}
		data->rbufs <<= 1;
		if (data->rbufs > CONN_MAXBUFSIZE)
			data->rbufs = CONN_MAXBUFSIZE;
		if ((ptr = realloc(data->rbuf, data->rbufs)) == NULL) {
			data->rbufs >>= 1;
			log_printfn(LOG_SERVER, "unable to increase receive buffer size, connection %x terminated", data->id);
			server_disconnect_nicely(data);
		} else {
			data->rbuf = ptr;
		}
	}

	if (data->rbufi != 0 && data->rbuf[data->rbufi - 1] == '\n') {
		data->rbuf[data->rbufi - 1] = '\0';
		chomp(data->rbuf);

		printf("debug: received \"%s\" on socket\n", data->rbuf);
		
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
	int r;
	struct connection *cd;
	struct sockaddr_storage peer_addr;
	socklen_t sin_size = sizeof(peer_addr);

	cd = malloc(sizeof(*cd));
	if (!cd) {
		log_printfn(LOG_SERVER, "failed creating connection data structure");
		return -1;
	}
	conn_init(cd);

	cd->peerfd = accept(fd, (struct sockaddr*)&peer_addr, &sin_size);
	if (cd->peerfd < 0) {
		r = errno;
		if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
			log_printfn(LOG_SERVER, "a peer disconnected before connection could be properly accepted");
			goto err_free;
		} else {
			log_printfn(LOG_SERVER, "could not accept socket connection: %s", strerror(errno));
			goto err_free;
		}
	}

	socklen_t len = sizeof(cd->sock);
	getpeername(cd->peerfd, (struct sockaddr*)&cd->sock, &len);
	pretty_print_peer(cd->peer, sizeof(cd->peer), cd->sock);
	log_printfn(LOG_SERVER, "new connection %x from %s", cd->id, cd->peer);

	pthread_rwlock_wrlock(&conn_list_lock);

	list_add_tail(&cd->list, &conn_list);
	ev_io_init(&cd->data_watcher, got_new_peer_data, cd->peerfd, EV_READ);
	ev_async_init(&cd->kill_watcher, server_disconnect_cb);
	cd->data_watcher.data = cd;
	cd->kill_watcher.data = cd;

	pthread_rwlock_unlock(&conn_list_lock);

	ev_async_start(loop, &cd->kill_watcher);

	log_printfn(LOG_SERVER, "serving new connection %x", cd->id);
	if (conn_fulfixinit(cd)) {
		log_printfn(LOG_SERVER, "unable to initialize connection\n");
		r = -1;
		goto err_stop;
	}

	ev_io_start(loop, &cd->data_watcher);

	return 0;

err_stop:
	pthread_rwlock_wrlock(&conn_list_lock);
	list_del(&cd->list);
	ev_async_stop(loop, &cd->kill_watcher);
	close(cd->peerfd);
	pthread_rwlock_unlock(&conn_list_lock);

err_free:
	connection_free(cd);
	free(cd);
	return r;
}

static void enable_accept_after_timer(struct ev_loop * const loop, ev_timer * const t, const int revents)
{
	ev_io *server_watcher = t->data;
	ev_timer_stop(loop, t);
	ev_io_start(loop, server_watcher);
	free(t);
	log_printfn(LOG_SERVER, "server now accepting connections again");
}

static void disable_accept_for_a_while(struct ev_loop * const loop, ev_io * const server_watcher, unsigned int const seconds)
{
		ev_io_stop(loop, server_watcher);

		ev_timer *t = malloc(sizeof(*t));
		memset(t, 0, sizeof(*t));

		ev_timer_init(t, enable_accept_after_timer, seconds, 0);
		t->data = server_watcher;
		ev_timer_start(loop, t);
}

#define SERVER_EMFILE_SLEEP 30
static void server_accept_cb(struct ev_loop * const loop, ev_io * const w, const int revents)
{
	int r;
	struct socket_list *s = w->data;
	r = server_accept_connection(loop, s->fd);

	if (r == EMFILE) {
		log_printfn(LOG_SERVER, "you should raise the ulimit for this process\n");
		log_printfn(LOG_SERVER, "disabling new connections for %d seconds to lessen system load", SERVER_EMFILE_SLEEP);
		disable_accept_for_a_while(loop, w, SERVER_EMFILE_SLEEP);
	}
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

static void* server_main(void *_server)
{
	struct server *server = _server;
	ev_io msg_watcher;
	loop = EV_DEFAULT;
	LIST_HEAD(sockets);
	LIST_HEAD(watchers);

	INIT_LIST_HEAD(&conn_list);
	pthread_rwlock_init(&conn_list_lock, NULL);

	signfdr = server->fd[0];
	signfdw = server->fd[1];

	if (conndata_init(&conn_data))
		die("%s", "failed initializing connection data structures");

	if (start_updating_ports())
		die("%s", "failed starting port update thread");

	initialize_server_sockets(&sockets);
	if (list_empty(&sockets))
		die("%s", "server failed to bind");

	start_server_watchers(&watchers, loop, &sockets);
	if (list_empty(&watchers))
		die("%s", "server failed to create watchers");

	ev_io_init(&msg_watcher, server_msg_cb, signfdr, EV_READ);

	ev_io_start(loop, &msg_watcher);

	log_printfn(LOG_SERVER, "server is up waiting for connections on port %s", SERVER_PORT);

	ev_run(loop, 0);

	ev_io_stop(loop, &msg_watcher);

	disconnect_peers(loop);
	stop_and_free_server_watchers(&watchers, loop);
	close_and_free_sockets(&sockets);
	stop_updating_ports();

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

int start_server(struct server * const server)
{
	sigset_t old, new;

	sigfillset(&new);

	if (pipe(server->fd) != 0)
		goto err;

	if (pthread_sigmask(SIG_SETMASK, &new, &old))
		goto err_close;

	if (pthread_create(&server->thread, NULL, server_main, server) != 0)
		goto err_close;

	if (pthread_sigmask(SIG_SETMASK, &old, NULL))
		goto err_cancel;

	return 0;

err_cancel:
	pthread_cancel(server->thread);
err_close:
	close(*server->fd);
err:
	return -1;
}

void stop_server(struct server * const server)
{
	struct signal signal = {
		.cnt = 0,
		.type = MSG_TERM
	};

	if (write(server->fd[1], &signal, sizeof(signal)) < 1)
		bug("%s", "server signalling fd seems closed when sending signal");

	log_printfn(LOG_MAIN, "waiting for server to terminate");
	pthread_join(server->thread, NULL);

	close(*server->fd);
}

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/queue.h>

struct connect_info {
	char *host;
	uint16_t port;
	int fd;
	struct sockaddr_in server_addr;
	SLIST_ENTRY(connect_info) list;
};
SLIST_HEAD(connect_head, connect_info) conn_list;

#define NUM_CONN 512
int initialize_connections(struct connect_head *list, char *host, uint16_t port)
{
	struct connect_info *info;

	for (unsigned int i = 0; i < NUM_CONN; i++) {
		info = malloc(sizeof(*info));
		assert(info);
		memset(info, 0, sizeof(*info));
		info->host = host;
		info->port = port;

		info->fd = socket(AF_INET, SOCK_STREAM, 0);
		assert(info->fd >= 0);

		memset(&info->server_addr, 0, sizeof(info->server_addr));
		info->server_addr.sin_family = AF_INET;
		info->server_addr.sin_port = htons(info->port);
		assert(inet_pton(AF_INET, info->host, &info->server_addr.sin_addr));

		SLIST_INSERT_HEAD(list, info, list);
	}

	return 0;
}

int establish_connections(struct connect_head *list)
{
	struct connect_info *info;

	SLIST_FOREACH(info, list, list) {
		printf("establishing\n");
		connect(info->fd, (struct sockaddr*)&info->server_addr, sizeof(info->server_addr));
		assert(info->fd > 0);
	}

	return 0;
}

static char data[] = "\n";
int write_on_connections(struct connect_head *list)
{
	struct connect_info *info;
	int n;

	SLIST_FOREACH(info, list, list) {
		printf("writing\n");
		n = write(info->fd, data, strlen(data));
		assert(n > 0);
		assert((unsigned int)n == strlen(data));
	}

	return 0;
}

int close_connections(struct connect_head *list)
{
	struct connect_info *info;

	SLIST_FOREACH(info, list, list) {
		printf("closing\n");
		close(info->fd);
	}

	return 0;
}

int free_connections(struct connect_head *list)
{
	struct connect_info *info;

	while (!SLIST_EMPTY(list)) {
		info = SLIST_FIRST(list);
		SLIST_REMOVE_HEAD(list, list);
		free(info);
	}

	return 0;
}

#define CONN_SLEEP 2
int main(int argc, char *argv[])
{

	if (argc != 3) {
		fprintf(stderr,
				"syntax:  %s <address> <port>\n"
				"purpose: establish %d simultaneous connections to address:port\n"
				"         keeping them open for %d seconds, then closing them.\n"
				"         This is useful for testing networked servers.\n",
				argv[0], NUM_CONN, CONN_SLEEP);
		exit(1);
	}

	long l;
	char *p;
	errno = 0;
	l = strtol(argv[2], &p, 10);
	if (errno != 0 || !p || *p != '\0' || l < 1 || l > UINT16_MAX) {
		fprintf(stderr, "port %s is not numeric or not in range\n", argv[2]);
		exit(1);
	}

	struct connect_head conn_list = SLIST_HEAD_INITIALIZER(conn_list);
	initialize_connections(&conn_list, argv[1], l);

	establish_connections(&conn_list);

	sleep(CONN_SLEEP);
	write_on_connections(&conn_list);

	sleep(CONN_SLEEP);
	write_on_connections(&conn_list);

	close_connections(&conn_list);
	free_connections(&conn_list);

	return 0;
}

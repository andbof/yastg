#ifndef _HAS_SERVER_H
#define _HAS_SERVER_H

#include "connection.h"

struct signal {
	uint16_t cnt;
	enum msg type;
} __attribute__((packed));

void server_disconnect_nicely(struct connection *conn);
void* server_main(void *p);

#endif

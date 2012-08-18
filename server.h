#ifndef HAS_SERVER_H
#define HAS_SERVER_H

struct signal {
	uint16_t cnt;
	enum msg type;
} __attribute__((packed));

void* server_main(void *p);

#endif

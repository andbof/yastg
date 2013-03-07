#ifndef _HAS_BUFFER_H
#define _HAS_BUFFER_H

struct buffer {
	char *buf;
	size_t idx;
	size_t size;
};

int read_into_buffer(const int fd, struct buffer * const buffer);
int write_buffer_into_fd(const int fd, struct buffer * const buffer);
int bufprintf(struct buffer * const buffer, char *format, ...);
int buffer_terminate_line(struct buffer * const buffer);
void buffer_reset(struct buffer *buffer);
void buffer_init(struct buffer * const buffer);
void buffer_free(struct buffer * const buffer);

#endif

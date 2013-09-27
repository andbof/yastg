#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "buffer.h"
#include "common.h"

#define BUFFER_MINSIZE 128
#define BUFFER_MAXSIZE 10240
static int enlarge_buffer(struct buffer * const buffer, size_t new_size)
{
	if (buffer->size >= BUFFER_MAXSIZE)
		return -1;

	new_size = MIN(new_size, BUFFER_MAXSIZE);
	new_size = MAX(new_size, BUFFER_MINSIZE);

	void *ptr;
	if (!(ptr = realloc(buffer->buf, new_size)))
		return -1;

	buffer->buf = ptr;
	buffer->size = new_size;

	return 0;
}

int read_into_buffer(const int fd, struct buffer * const buffer)
{
	assert(buffer);
	assert(fd >= 0);

	if (!buffer->size) {
		buffer->buf = malloc(BUFFER_MINSIZE);
		if (!buffer->buf)
			return -1;

		buffer->size = BUFFER_MINSIZE;
	}

	ssize_t r;
	r = read(fd, buffer->buf + buffer->idx, buffer->size - buffer->idx);
	if (r < 0)
		return r;

	buffer->idx += r;

	if ((buffer->idx >= buffer->size) && (buffer->buf[buffer->idx - 1] != '\n')) {
		if (enlarge_buffer(buffer, buffer->size * 2))
			return -1;
	}

	return r;
}

int write_buffer_into_fd(const int fd, struct buffer * const buffer)
{
	assert(buffer);
	assert(fd >= 0);
	ssize_t r;
	size_t sb = 0;

	if (!buffer->idx)
		return 0;

	do {
		r = write(fd, buffer->buf + sb, buffer->idx - sb);
		if (r < 1)
			return r;

		sb += r;
	} while (sb < buffer->idx);

	buffer->idx = 0;

	return sb;
}

__attribute__((format(printf, 2, 0)))
int vbufprintf(struct buffer * const buffer, const char *fmt, va_list ap)
{
	va_list _ap;
	size_t size, len;
	assert(buffer);

	do {
		size = buffer->size - buffer->idx;

		va_copy(_ap, ap);
		len = vsnprintf(buffer->buf + buffer->idx, size, fmt, _ap);
		va_end(_ap);

		if (len >= size && enlarge_buffer(buffer, buffer->size * 2))
			return -1;
	} while (len >= size);

	buffer->idx += len;

	return 0;

}

__attribute__((format(printf, 2, 3)))
int bufprintf(struct buffer * const buffer, char *format, ...)
{
	va_list ap;
	int r;

	va_start(ap, format);
	r = vbufprintf(buffer, format, ap);
	va_end(ap);

	return r;
}

int buffer_terminate_line(struct buffer * const buffer)
{
	assert(buffer);

	if (buffer->idx == 0 || buffer->buf[buffer->idx - 1] != '\n')
		return -1;

	buffer->buf[buffer->idx - 1] = '\0';
	chomp(buffer->buf);
	return 0;
}

void buffer_reset(struct buffer * const buffer)
{
	buffer->idx = 0;
}

void buffer_init(struct buffer * const buffer)
{
	memset(buffer, 0, sizeof(*buffer));
}

void buffer_free(struct buffer * const buffer)
{
	free(buffer->buf);
	memset(buffer, 0, sizeof(*buffer));
}

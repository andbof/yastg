#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "asciiart.h"

int szprintf(char *out, char *format, ...)
{
	va_list ap;
	int n;

	va_start(ap, format);
	n = vsprintf(out, format, ap);
	va_end(ap);

	out[n] = out[n + 1];

	return n;
}

int draw_square(const size_t buf_size_x, const size_t buf_size_y,
		char buf[buf_size_y][buf_size_x],
		const size_t top_x, const size_t top_y, const size_t width_x)
{
	const size_t width_y = width_x / ASCII_X_TO_Y;
	size_t x, y;

	if (width_x > buf_size_x || width_y > buf_size_y)
		return -1;

	for (y = 0; y < width_y; y++)
		memset(&buf[y][0], ' ', width_x);

	for (x = 0; x < width_x; x++) {
		buf[0][x] = '-';
		buf[width_y - 1][x] = '-';
	}

	for (y = 1; y < width_y; y++) {
		buf[y][0] = '|';
		buf[y][width_x - 1] = '|';
	}

	buf[0][0] = '/';
	buf[0][width_x - 1] = '\\';
	buf[width_y - 1][0] = '\\';
	buf[width_y - 1][width_x - 1] = '/';

	return 0;
}

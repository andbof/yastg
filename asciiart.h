#ifndef _HAS_ASCIIART_H
#define _HAS_ASCIIART_H

#define ASCII_X_TO_Y 2.25

int szprintf(char *out, char *format, ...);
int draw_square(const size_t buf_size_x, const size_t buf_size_y,
		char buf[buf_size_y][buf_size_x],
		const size_t top_x, const size_t top_y, const size_t width);

#endif

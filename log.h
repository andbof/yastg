#ifndef _HAS_LOG_H
#define _HAS_LOG_H

void log_init(const char * const name);
void log_init_stdout();
void log_close();
void log_printfn(const char *subsys, const char *format, ...);

#endif

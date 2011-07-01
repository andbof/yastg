#ifndef HAS_LOG_H
#define HAS_LOG_H

#define LOG_NAME "yastg.log"

void log_init();
void log_close();
void log_printfn(const char *subsys, const char *format, ...);

#endif

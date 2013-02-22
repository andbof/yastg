#ifndef _HAS_LOG_H
#define _HAS_LOG_H

enum log_subsystems {
	LOG_CONFIG,
	LOG_CONN,
	LOG_MAIN,
	LOG_PANIC,
	LOG_PORT_UPDATE,
	LOG_SERVER,
	LOG_SUBSYSTEM_NUM
};

void log_init(const char * const name);
void log_init_stdout();
void log_close();
void __attribute__((format(printf, 2, 3))) log_printfn(const enum log_subsystems subsystem,
		const char *format, ...);

#endif

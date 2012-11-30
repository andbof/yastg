#ifndef _HAS_MODULE_H
#define _HAS_MODULE_H

#include "list.h"

#define MODULE_MAX_NAME_LEN 16
#define MODULE_STRUCT_VERSION 0
#define MODULE(mod_name, init_func, exit_func)			\
	static struct module _module_struct_##mod_name;		\
	struct module* _get_module_struct(void)			\
	{							\
		return &_module_struct_##mod_name;		\
	}							\
	static struct module _module_struct_##mod_name = {	\
		.struct_version = MODULE_STRUCT_VERSION,	\
		.name = #mod_name,				\
		.init_function = init_func,			\
		.exit_function = exit_func			\
	}

struct module {
	int struct_version;
	char name[MODULE_MAX_NAME_LEN];
	int (*init_function)(void);
	void (*exit_function)(void);
	void *dl;
	unsigned int size;
	struct list_head list;
};

enum module_error {
	MODULE_SUCCESS = 0,
	MODULE_DL_ERROR,
	MODULE_NO_STRUCT,
	MODULE_NOT_SANE,
	MODULE_ALREADY_LOADED,
	MODULE_FAILED_STAT,
	MODULE_FAILED_INIT,
	/* Only add above this line */
	MODULE_NUM_ERR
};

extern struct list_head modules_loaded;

const char* module_strerror(const enum module_error error);
enum module_error module_insert(const char * const name);
enum module_error module_remove(struct module *m);

#endif

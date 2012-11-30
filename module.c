#include <dlfcn.h>
#include <string.h>
#include <sys/stat.h>
#include "module.h"

LIST_HEAD(modules_loaded);

static int is_module_sane(struct module *m)
{
	if (m->struct_version != MODULE_STRUCT_VERSION)
		return 0;

	if (!m->init_function)
		return 0;

	if (!m->exit_function)
		return 0;

	if (strlen(m->name) < 1)
		return 0;

	for (char *p = m->name; *p != '\0'; p++) {
		switch (*p) {
		case '-':
			continue;
		case '_':
			continue;
		case '0' ... '9':
			continue;
		case 'A' ... 'Z':
			continue;
		case 'a' ... 'z':
			continue;
		default:
			return 0;
		}
	}

	return 1;
}

static int is_module_loaded(const char * const name)
{
	struct module *_m;
	list_for_each_entry(_m, &modules_loaded, list)
		if (strcmp(_m->name, name) == 0)
			return 1;

	return 0;
}

static const char strerrors[MODULE_NUM_ERR][64] = {
	"Success",
	"Error return from dl function",
	"Error when access module struct",
	"Module is not sane",
	"Module already loaded",
	"Failed stat()",
	"Module init function returned error",
};

const char* module_strerror(const enum module_error error)
{
	if (error >= MODULE_NUM_ERR)
		return NULL;

	return strerrors[error];
}

enum module_error module_insert(const char * const name)
{
	void *dl;
	struct module *m;
	struct module* (*get_struct)(void);
	int r;

	dl = dlopen(name, RTLD_LAZY);
	if (!dl)
		return MODULE_DL_ERROR;

	get_struct = dlsym(dl, "_get_module_struct");
	if (!get_struct) {
		r = MODULE_DL_ERROR;
		goto err_close;
	}

	m = get_struct();
	if (!m) {
		r = MODULE_NO_STRUCT;
		goto err_close;
	}

	if (!is_module_sane(m)) {
		r = MODULE_NOT_SANE;
		goto err_close;
	}

	if (is_module_loaded(m->name)) {
		r = MODULE_ALREADY_LOADED;
		goto err_close;
	}

	struct stat stat_data;
	if (stat(name, &stat_data)) {
		r = MODULE_FAILED_STAT;
		goto err_close;
	}

	m->dl = dl;
	m->size = stat_data.st_size;

	if (m->init_function()) {
		r = MODULE_FAILED_INIT;
		goto err_close;
	}

	list_add_tail(&m->list, &modules_loaded);

	return 0;

err_close:
	dlclose(dl);
	return r;
}

enum module_error module_remove(struct module *m)
{
	int r;

	m->exit_function();
	list_del(&m->list);

	r = dlclose(m->dl);
	if (r)
		return MODULE_DL_ERROR;

	return 0;
}

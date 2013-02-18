#ifndef _HAS_PORT_TYPE_H
#define _HAS_PORT_TYPE_H

#include "list.h"
#include "ptrlist.h"

enum port_zone {
	OCEAN,
	SURFACE,
	ORBIT,
	ROGUE,
	PORT_ZONE_NUM
};

extern char port_zone_names[PORT_ZONE_NUM][8];

struct port_type {
	char *name;
	char *desc;
	struct list_head list;
	struct list_head items;
	struct list_head item_names;
	int zones[PORT_ZONE_NUM];
};

int load_all_ports();
void port_type_free(struct port_type *type);

#endif

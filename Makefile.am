ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -D_GNU_SOURCE
AM_CFLAGS = -g -O0 -Wall -Werror -Wformat -Wformat-security -Wformat-nonliteral -Wformat=2 -ftrapv -Wno-unused-parameter -Wtype-limits -fstack-protector-all -fno-strict-aliasing -Wno-format-y2k
AM_YFLAGS = -d

TESTS = test/cli_test \
	test/config_test \
	test/ptrlist_test \
	test/stringtree_test
BUILT_SOURCES = parseconfig-yacc.c parseconfig-lex.c

confdir = $(sysconfdir)/xdg/yastg
bin_PROGRAMS = yastg
check_PROGRAMS = test/cli_test \
		 test/config_test \
		 test/conntest \
		 test/ptrlist_test \
		 test/stringtree_test
check_LTLIBRARIES = test_module.la
dist_conf_DATA = data/constellations \
		 data/firstnames \
		 data/foociv \
		 data/grazny \
		 data/items \
		 data/placenames \
		 data/placeprefix \
		 data/placesuffix \
		 data/planets \
		 data/ports \
		 data/ships \
		 data/surnames \
		 data/terran \
		 data/yastg.conf

# optflags: -O3 -funroll-loops
yastg_SOURCES = asciiart.c \
		asciiart.h \
		buffer.c \
		buffer.h \
		cargo.c \
		cargo.h \
		civ.c \
		civ.h \
		cli.c \
		cli.h \
		common.c \
		common.h \
		connection.c \
		connection.h \
		constellation.c \
		constellation.h \
		console.c \
		console.h \
		inventory.h \
		item.c \
		item.h \
		list.h \
		loadconfig.c \
		loadconfig.h \
		log.c \
		log.h \
		main.c \
		map.c \
		map.h \
		module.c \
		module.h \
		mt19937ar-cok.c \
		mt19937ar-cok.h \
		mtrandom.c \
		mtrandom.h \
		names.c \
		names.h \
		parseconfig-lex.l \
		parseconfig-yacc.y \
		parseconfig.h \
		parseconfig-rename.h \
		planet.c \
		planet.h \
		planet_type.c \
		planet_type.h \
		player.c \
		player.h \
		port.c \
		port.h \
		port_type.c \
		port_type.h \
		port_update.c \
		port_update.h \
		ptrarray.c \
		ptrarray.h \
		ptrlist.c \
		ptrlist.h \
		rbtree.c \
		rbtree.h \
		system.c \
		system.h \
		server.c \
		server.h \
		ship.c \
		ship.h \
		ship_type.c \
		ship_type.h \
		star.c \
		star.h \
		stringtree.c \
		stringtree.h \
		universe.c \
		universe.h

test_conntest_SOURCES = test/conntest.c

test_cli_test_SOURCES = test/cli_test.c \
			cli.c \
			cli.h \
			common.c \
			common.h \
			stringtree.c \
			stringtree.h

test_ptrlist_test_SOURCES = test/ptrlist_test.c \
			    mt19937ar-cok.c \
			    mtrandom.c \
			    ptrlist.c

test_stringtree_test_SOURCES = test/stringtree_test.c \
			       stringtree.c \
			       common.c

test_config_test_SOURCES = test/config_test.c \
			   log.c \
			   log.h \
			   parseconfig.h \
			   parseconfig-lex.l \
			   parseconfig-rename.h \
			   parseconfig-yacc.y

# The LDFLAGS for test_module are required to make a shared library without the
# 'lib' prefix (-module), without any version suffix (-avoid-version) and force
# building a shared library even though it isn't installed (-rpath /dev/null)
test_module_la_LDFLAGS = -module -avoid-version -rpath /dev/null
test_module_la_SOURCES = test/test_module.c

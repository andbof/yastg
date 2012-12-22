MAINTAINERCLEANFILES = Makefile.in \
		       aclocal.m4 \
		       config.h.in \
		       configure \
		       depcomp \
		       missing \
		       install-sh \
		       config.guess \
		       config.h.in~ \
		       config.sub \
		       ltmain.sh \
		       libtool \
		       m4/*.m4

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -D_GNU_SOURCE
AM_CFLAGS = -g -O0 -Wall -Werror -Wformat -Wformat-security -Wformat-nonliteral -Wformat=2 -ftrapv -Wno-unused-variable -Wno-unused-parameter -Wtype-limits -fstack-protector-all -fno-strict-aliasing -Wno-format-y2k
AM_YFLAGS = -d

TESTS = test/stringtree_test test/config_test
BUILT_SOURCES = parseconfig-yacc.c parseconfig-lex.c

bin_PROGRAMS = yastg
check_PROGRAMS = test/stringtree_test test/config_test test/conntest
check_LTLIBRARIES = test_module.la

# optflags: -O3 -funroll-loops
yastg_SOURCES = base.c \
		base.h \
		base_type.c \
		base_type.h \
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
		inventory.h \
		item.c \
		item.h \
		list.h \
		log.c \
		log.h \
		main.c \
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

test_stringtree_test_SOURCES = test/stringtree_test.c \
			       stringtree.c \
			       common.c

test_config_test_SOURCES = test/config_test.c \
			   log.c \
			   log.h \
			   parseconfig-lex.l \
			   parseconfig-yacc.y

# The LDFLAGS for test_module are required to make a shared library without the
# 'lib' prefix (-module), without any version suffix (-avoid-version) and force
# building a shared library even though it isn't installed (-rpath /dev/null)
test_module_la_LDFLAGS = -module -avoid-version -rpath /dev/null
test_module_la_SOURCES = test/test_module.c

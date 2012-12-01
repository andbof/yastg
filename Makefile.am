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

CFLAGS = -g -O0 -Wall -Werror -Wformat -Wformat-security -Wformat-nonliteral -Wformat=2 -ftrapv -Wno-unused-variable -Wno-unused-parameter -Wtype-limits -fstack-protector-all -fno-strict-aliasing

TESTS = test/sarraytest test/stringtree_test

bin_PROGRAMS = yastg
check_PROGRAMS = test/sarraytest test/stringtree_test test/conntest
check_LTLIBRARIES = test_module.la

# optflags: -O3 -funroll-loops
yastg_SOURCES = base.c \
		base.h \
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
		data.c \
		data.h \
		id.c \
		id.h \
		inventory.h \
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
		parseconfig.c \
		parseconfig.h \
		planet.c \
		planet.h \
		player.c \
		player.h \
		ptrarray.c \
		ptrarray.h \
		ptrlist.c \
		ptrlist.h \
		sarray.c \
		sarray.h \
		sector.c \
		sector.h \
		server.c \
		server.h \
		star.c \
		star.h \
		stringtree.c \
		stringtree.h \
		universe.c \
		universe.h

test_sarraytest_SOURCES = test/sarraytest.c \
			  sarray.c \
			  mt19937ar-cok.c \
			  mtrandom.c \
			  id.c \
			  log.c

test_conntest_SOURCES = test/conntest.c

test_stringtree_test_SOURCES = test/stringtree_test.c \
			       stringtree.c \
			       common.c

# The LDFLAGS for test_module are required to make a shared library without the
# 'lib' prefix (-module), without any version suffix (-avoid-version) and force
# building a shared library even though it isn't installed (-rpath /dev/null)
test_module_la_LDFLAGS = -module -avoid-version -rpath /dev/null
test_module_la_SOURCES = test/test_module.c

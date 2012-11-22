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

CFLAGS = -g -O0 -Wall -Wformat -Wformat-security -Wformat-nonliteral -Wformat=2 -ftrapv -Wno-unused-variable -Wno-unused-parameter -Wtype-limits -fstack-protector-all -fno-strict-aliasing

TESTS = test/sarraytest test/stacktest

bin_PROGRAMS = yastg
check_PROGRAMS = test/sarraytest test/stacktest

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
		htable.c \
		htable.h \
		id.c \
		id.h \
		inventory.h \
		list.h \
		log.c \
		log.h \
		main.c \
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
		stack.c \
		stack.h \
		star.c \
		star.h \
		universe.c \
		universe.h

test_sarraytest_SOURCES = test/sarraytest.c \
			  sarray.c \
			  mt19937ar-cok.c \
			  mtrandom.c \
			  id.c \
			  log.c

test_stacktest_SOURCES = test/stacktest.c \
			 stack.c \
			 mt19937ar-cok.c \
			 mtrandom.c \
			 log.c

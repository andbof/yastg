AC_PREREQ([2.63])
AC_INIT([yastg],[m4_esyscmd_s([git describe --always])])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
AC_CONFIG_SRCDIR([main.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_FILES([Makefile])

AC_PROG_CC
AC_PROG_CC_STDC
AM_PROG_CC_C_O
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])    # Fixes automake 1.12 (where -Wall implies portability warnings), but automake <= 1.11 does not recognize it
AC_LANG_C
AC_PROG_LIBTOOL
AC_PROG_YACC
AM_PROG_LEX

AC_CHECK_LIB([m],[floor],
             [],
             [AC_MSG_ERROR([libm not found])])
AC_CHECK_LIB([pthread],[pthread_create],
             [],
             [AC_MSG_ERROR([libpthread not found])])

AC_CHECK_LIB([ev],[ev_now],
	     [],
	     [AC_MSG_ERROR([libev not found])])
AC_CHECK_FUNC([ev_run],
	      [],
	      [AC_DEFINE([ev_run], [ev_loop], [in libev3 in contrast to libev4, ev_run is called ev_loop])])

AC_CHECK_LIB([dl],[dlopen],
	     [],
	     [AC_MSG_ERROR([libdl not found])])

AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h malloc.h netdb.h netinet/in.h \
                  stddef.h stdint.h stdlib.h string.h sys/file.h sys/socket.h \
                  unistd.h],
                 [],
                 [AC_MSG_ERROR([A standard header file was not found])])

AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T

AC_FUNC_ALLOCA
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([floor memmove memset munmap select socket sqrt strchr strdup \
                strerror],
               [],
               [AC_MSG_ERROR([A standard library function was not found])])

AC_OUTPUT

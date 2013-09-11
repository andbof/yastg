AC_PREREQ([2.67])
AC_INIT([yastg],[m4_esyscmd_s([git describe --always --dirty])])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
AC_CONFIG_SRCDIR([main.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_FILES([Makefile])

AC_PROG_AWK
AC_PROG_CC
AC_PROG_CC_C99
if test "x$ac_cv_prog_cc_c99" = "xno"; then
	AC_MSG_ERROR([A c99 compiler is required])
fi
AM_PROG_CC_C_O
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])    # Fixes automake 1.12 (where -Wall implies portability warnings), but automake <= 1.11 does not recognize it
AC_LANG_C
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_YACC
AM_PROG_LEX

AC_CHECK_LIB([m],[floor],
             [],
             [AC_MSG_ERROR([libm not found])])
AC_CHECK_LIB([pthread],[pthread_create],
             [],
             [AC_MSG_ERROR([libpthread not found])])
AC_CHECK_LIB([rt],[clock_gettime],
	     [],
	     [AC_MSG_ERROR([librt not found])])

AX_LIB_EV

AC_CHECK_LIB([dl],[dlopen],
	     [],
	     [AC_MSG_ERROR([libdl not found])])

AC_CHECK_LIB([xdg-basedir],[xdgInitHandle],
	     [],
	     [AC_MSG_ERROR([libxdg-basedir not found])])

AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h malloc.h netdb.h netinet/in.h \
                  stddef.h stdint.h stdlib.h string.h sys/file.h sys/socket.h \
                  unistd.h],
                 [],
                 [AC_MSG_ERROR([A standard header file was not found])])

AX_C___ATTRIBUTE__
if test "x$ax_cv___attribute__" != "xyes"; then
	AC_MSG_ERROR([A compiler that supports __attribute__ is required])
fi

AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

AC_FUNC_ALLOCA
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([clock_gettime floor localtime_r memset munmap pow socket sqrt \
		strchr strdup strerror strstr strtol],
               [],
               [AC_MSG_ERROR([A standard library function was not found])])

AC_OUTPUT

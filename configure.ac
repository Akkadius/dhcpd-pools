#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.64)
AC_CONFIG_MACRO_DIR([m4])
AC_INIT([dhcpd-pools],
	m4_esyscmd([build-aux/git-version-gen .tarball-version]),
	[kerolasa@iki.fi],,[http://dhcpd-pools.sourceforge.net/])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([-Wall foreign 1.10 tar-pax no-dist-gzip dist-xz -Wextra-portability])
AC_CONFIG_SRCDIR([src/dhcpd-pools.h])
AC_CONFIG_HEADERS([config.h])

AC_GNU_SOURCE

# Checks for programs
AC_PROG_AWK
AC_PROG_CC_C99
gl_EARLY
gl_INIT
AC_C_RESTRICT
AC_PROG_CXX
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

# Checks for libraries.
LT_INIT

# Checks for header files.
AC_CHECK_HEADERS([\
	arpa/inet.h \
	fcntl.h \
	libintl.h \
	limits.h \
	sys/socket.h \
])

AC_ARG_WITH(uthash,
AC_HELP_STRING([--with-uthash=DIR], [Use uthash from [DIR]/uthash.h)]),
[ case "$with_uthash" in
  yes)
    ;;
  no)
    AC_MSG_ERROR([the uthash.h is required])
    ;;
  *)
    CPPFLAGS="$CPPFLAGS -I$with_uthash"
    ;;
  esac]
)
AC_CHECK_HEADER(uthash.h, [], [AC_MSG_ERROR([Unable to find uthash.h])])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_STRTOD
AC_CHECK_FUNCS([\
	__fpending \
	atexit \
	inet_ntoa \
	memset \
	posix_fadvise \
	strdup \
	strrchr \
])

AC_MSG_CHECKING([if the compiler supports __builtin_expect])
AC_TRY_COMPILE(, [
    return __builtin_expect(1, 1) ? 1 : 0
], [
    have_builtin_expect=yes
    AC_MSG_RESULT([yes])
], [
    have_builtin_expect=no
    AC_MSG_RESULT([no])
])
if test x$have_builtin_expect = xyes ; then
    AC_DEFINE([HAVE_BUILTIN_EXPECT], [1], [Define to 1 if the compiler supports __builtin_expect.])
fi

AC_CONFIG_FILES([Makefile
		 lib/Makefile
		 man/Makefile
		 src/Makefile
		 contrib/Makefile])

AC_MSG_CHECKING(whether program_invocation_short_name is defined)
AC_TRY_COMPILE([#include <argp.h>],
		[program_invocation_short_name = "test";],
		AC_DEFINE(HAVE_PROGRAM_INVOCATION_SHORT_NAME, 1,
			[Define if program_invocation_short_name is defined])
		AC_MSG_RESULT(yes),
		AC_MSG_RESULT(no))

AC_MSG_CHECKING([whether __progname is defined])
AC_LINK_IFELSE([AC_LANG_PROGRAM([extern char *__progname;],
	[if (*__progname == 0) return;])],
	AC_DEFINE(HAVE___PROGNAME, 1, [Define if __progname is defined])
	AC_MSG_RESULT(yes),
	AC_MSG_RESULT(no))

AC_ARG_WITH(dhcpd-conf,
	[AC_HELP_STRING([--with-dhcpd-conf], [default path of dhcpd.conf])],
	AC_DEFINE_UNQUOTED(DHCPDCONF_FILE,"$withval"),
	AC_DEFINE(DHCPDCONF_FILE,["/etc/dhcpd.conf"],[default path of dhcpd.conf]))
AC_ARG_WITH(dhcpd-leases,
	[AC_HELP_STRING([--with-dhcpd-leases], [default path of dhcpd.leases])],
	AC_DEFINE_UNQUOTED(DHCPDLEASE_FILE,"$withval"),
	AC_DEFINE(DHCPDLEASE_FILE,["/var/lib/dhcp/dhcpd.leases"],[default path of dhcpd.leases]))
AC_ARG_WITH(output-format,
	[AC_HELP_STRING([--with-output-format], [default output format])],
	AC_DEFINE_UNQUOTED(OUTPUT_FORMAT,"$withval"),
	AC_DEFINE(OUTPUT_FORMAT,["text"],[default output format]))
AC_ARG_WITH(output-limit,
	[AC_HELP_STRING([--with-output-limit], [default output limitations])],
	AC_DEFINE_UNQUOTED(OUTPUT_LIMIT,"$withval"),
	AC_DEFINE(OUTPUT_LIMIT,["77"],[default output limitations]))

AC_OUTPUT

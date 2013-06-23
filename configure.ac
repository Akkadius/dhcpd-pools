# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_CONFIG_MACRO_DIR([m4])
AC_INIT([dhcpd-pools],
	[m4_esyscmd([build-aux/git-version-gen .tarball-version])],
	[kerolasa@iki.fi],[],
	[http://dhcpd-pools.sourceforge.net/])
PACKAGE_MAINTAINER="Sami Kerola"
AC_SUBST([PACKAGE_MAINTAINER])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([
	-Wall
	-Wextra-portability
	1.10
	foreign
	dist-xz
	no-dist-gzip
	parallel-tests
	subdir-objects
	tar-pax
])
AC_CONFIG_SRCDIR([src/dhcpd-pools.h])
AC_CONFIG_HEADERS([config.h])

# Checks for programs
AC_USE_SYSTEM_EXTENSIONS
AC_C_RESTRICT
AC_PROG_CC_C99
gl_EARLY
gl_INIT
AC_PROG_MAKE_SET
LT_INIT

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

AC_ARG_WITH(
	[uthash],
	[AS_HELP_STRING([--with-uthash=DIR], [Use uthash from [DIR]/uthash.h)])],
	[AS_CASE([$with_uthash],
		[yes], [],
		[no], [AC_MSG_ERROR([the uthash is required])],
		[CPPFLAGS="$CPPFLAGS -I$with_uthash"]
	)]
)
AC_CHECK_HEADER([uthash.h], [], [AC_MSG_ERROR([Unable to find uthash.h])])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_CHECK_FUNCS([\
	__fpending \
	posix_fadvise \
])

AC_MSG_CHECKING([if the compiler supports __builtin_expect])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[
	return __builtin_expect(1, 1) ? 1 : 0
]])],[
	have_builtin_expect=yes
	AC_MSG_RESULT([yes])
],[
	have_builtin_expect=no
	AC_MSG_RESULT([no])
])
AS_IF([test x$have_builtin_expect = xyes], [
	AC_DEFINE([HAVE_BUILTIN_EXPECT], [1],
		[The compiler supports __builtin_expect])
])

AC_ARG_WITH([dhcpd-conf],
	[AS_HELP_STRING([--with-dhcpd-conf=FILE],[default path of dhcpd.conf])],
	[dhcpd_conf_path="$withval"],
	[dhcpd_conf_path="/etc/dhcpd.conf"])
AC_DEFINE_UNQUOTED([DHCPDCONF_FILE], ["$dhcpd_conf_path"], [default path of dhcpd.conf])
AC_SUBST([DHCPDCONF_FILE], [$dhcpd_conf_path])

AC_ARG_WITH([dhcpd-leases],
	[AS_HELP_STRING([--with-dhcpd-leases=FILE],[default path of dhcpd.leases])],
	[dhcpd_leases_path="$withval"],
	[dhcpd_leases_path="/var/lib/dhcp/dhcpd.leases"])
AC_DEFINE_UNQUOTED([DHCPDLEASE_FILE], ["$dhcpd_leases_path"], [default path of dhcpd.leases])
AC_SUBST([DHCPDLEASE_FILE], [$dhcpd_leases_path])

AC_ARG_WITH([output-format],
	[AS_HELP_STRING([--with-output-format=FORMAT],[default output format])],
	[output_format="$withval"],
	[output_format="text"])
AC_DEFINE_UNQUOTED([OUTPUT_FORMAT], ["$output_format"], [default output format])
AC_SUBST([OUTPUT_FORMAT], [$output_format])

AC_ARG_WITH([output-limit],
	[AS_HELP_STRING([--with-output-limit=MASK],[default output limit])],
	[output_limit="$withval"],
	[output_limit="77"])
AC_DEFINE_UNQUOTED([OUTPUT_LIMIT], ["$output_limit"], [default output limit])
AC_SUBST([OUTPUT_LIMIT], [$output_limit])

AC_ARG_WITH([warning],
	[AS_HELP_STRING([--with-warning=NUM],[monitoring warning default])],
	[warning_percent="$withval"],
	[warning_percent="80"])
AC_DEFINE_UNQUOTED([ALARM_WARN], [$warning_percent], [monitoring warning default])
AC_SUBST([ALARM_WARN], [$warning_percent])

AC_ARG_WITH([critical],
	[AS_HELP_STRING([--with-critical=NUM],[monitoring critical default])],
	[critical_percent="$withval"],
	[critical_percent="90"])
AC_DEFINE_UNQUOTED([ALARM_CRIT], [$critical_percent], [monitoring critical default])
AC_SUBST([ALARM_CRIT], [$critical_percent])

AC_ARG_ENABLE([doxygen],
	AS_HELP_STRING([--enable-doxygen],[build internal api documentation @<:@default=disabled@:>@]),
	[], [enable_doxygen=no])
AM_CONDITIONAL([ENABLE_DOXYGEN],  [test "x$enable_doxygen" = "xyes"])

AS_IF([test x$enable_doxygen = xyes], [
	AC_CHECK_PROGS([DOXYGEN], [doxygen])
	AS_IF([test "x$DOXYGEN" = "x"],
		AC_MSG_ERROR([doxygen not in path]),
		AC_CONFIG_FILES([doc/doxy.conf])
	)

	AC_CHECK_PROGS([DOXYGEN_DOT], [dot])
		AS_IF([test "x$DOXYGEN" = "x"],
		AC_MSG_WARN([dot not in path, internal documentation graphs are not generated])
	)
])
AM_CONDITIONAL([HAVE_DOXYGEN], [test "x$DOXYGEN" != "x"])
AM_CONDITIONAL([HAVE_DOXYGEN_DOT], [test "x$DOXYGEN_DOT" != "x"])

AC_CONFIG_FILES([
	Makefile
	lib/Makefile
	man/dhcpd-pools.1
])

AC_OUTPUT

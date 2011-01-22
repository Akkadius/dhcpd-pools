#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.64)
AC_CONFIG_MACRO_DIR([m4])
AC_INIT([dhcpd-pools],[2.13],[kerolasa@iki.fi],,[http://dhcpd-pools.sourceforge.net/])
AM_INIT_AUTOMAKE(dhcpd-pools, 2.13)
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

AC_GNU_SOURCE

# Checks for programs
AC_PROG_CC

# Checks for libraries.
LT_INIT

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h libintl.h netinet/in.h stdlib.h string.h strings.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([inet_ntoa memset strerror strstr])

# GNU commandline parsing in getopt.h present?
AC_CHECK_HEADERS(getopt.h, AC_DEFINE(HAVE_GETOPT_H),
        import_sources="getopt.c getopt1.c $import_sources")

AC_CONFIG_FILES([Makefile
                 man/Makefile
                 src/Makefile
                 contrib/Makefile])

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

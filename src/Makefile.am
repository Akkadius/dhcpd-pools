## Makefile.am -- Process this file with automake to produce Makefile.in

bin_PROGRAMS = dhcpd-pools
AC_PROG_RANLIB = resolv
AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/lib -I$(top_builddir)/lib

dhcpd_pools_LDADD = $(top_builddir)/lib/libdhcpd_pools.la

dhcpd_pools_SOURCES = \
	analyze.c \
	defaults.h \
	dhcpd-pools.c \
	dhcpd-pools.h \
	getdata.c \
	hash.c \
	other.c \
	output.c \
	sort.c

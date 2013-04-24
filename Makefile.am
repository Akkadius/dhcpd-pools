## Makefile.am -- Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = gnu
ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = \
	.version \
	build-aux/git-version-gen \
	m4/gnulib-cache.m4

SUBDIRS = lib

BUILT_SOURCES = $(top_srcdir)/.version
$(top_srcdir)/.version:
	echo $(VERSION) > $@-t && mv $@-t $@
dist-hook:
	echo $(VERSION) > $(distdir)/.tarball-version

CLEANFILES =
CLEAN_LOCALS =

include contrib/Makemodule.am
include doc/Makemodule.am
include man/Makemodule.am
include src/Makemodule.am
include tests/Makemodule.am

clean-local: $(CLEAN_LOCALS)

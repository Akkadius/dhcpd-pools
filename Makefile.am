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

PATHFILES =
CLEANFILES = $(PATHFILES)
EXTRA_DIST += $(PATHFILES:=.in)
CLEAN_LOCALS =

edit_cmd = sed \
	 -e 's|@ALARM_CRIT[@]|$(ALARM_CRIT)|g' \
	 -e 's|@ALARM_WARN[@]|$(ALARM_WARN)|g' \
	 -e 's|@DHCPDCONF_FILE[@]|$(DHCPDCONF_FILE)|g' \
	 -e 's|@DHCPDLEASE_FILE[@]|$(DHCPDLEASE_FILE)|g' \
	 -e 's|@OUTPUT_FORMAT[@]|$(OUTPUT_FORMAT)|g' \
	 -e 's|@OUTPUT_LIMIT[@]|$(OUTPUT_LIMIT)|g' \
	 -e 's|@PACKAGE_BUGREPORT[@]|$(PACKAGE_BUGREPORT)|g' \
	 -e 's|@PACKAGE_MAINTAINER[@]|$(PACKAGE_MAINTAINER)|g' \
	 -e 's|@PACKAGE_NAME[@]|$(PACKAGE_NAME)|g' \
	 -e 's|@PACKAGE_URL[@]|$(PACKAGE_URL)|g' \
	 -e 's|@SHELL[@]|$(SHELL)|g' \
	 -e 's|@VERSION[@]|$(VERSION)|g' \
	 -e 's|@bindir[@]|$(bindir)|g' \
	 -e 's|@top_srcdir[@]|$(top_srcdir)|g'

$(PATHFILES): Makefile
	@ rm -f $@ $@.tmp
	$(AM_V_at) test -d $(dir $@) || mkdir -p $(dir $@)
	$(AM_V_GEN) srcdir=''; \
		test -f ./$@.in || srcdir=$(srcdir)/; \
		$(edit_cmd) $${srcdir}$@.in >$@.tmp
	@ mv $@.tmp $@

include contrib/Makemodule.am
include doc/Makemodule.am
include man/Makemodule.am
include src/Makemodule.am
include tests/Makemodule.am

clean-local: $(CLEAN_LOCALS)

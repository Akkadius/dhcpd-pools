EXTRA_DIST = \
	bootp \
	confs \
	expected \
	leading0 \
	leases \
	simple \
	test.sh

TESTS = simple bootp leading0
check_SCRIPTS = test.sh

check-local: $(SHELL) $(top_srcdir)/tests/test.sh

clean-local:
	rm -rf $(top_builddir)/tests/outputs

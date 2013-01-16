TESTS = \
	bootp \
	complete \
	full-json \
	leading0 \
	same-twice \
	simple \
	v6

EXTRA_DIST = \
	confs \
	expected \
	leases \
	$(TESTS)

TESTS_ENVIRONMENT = top_srcdir=$(top_srcdir) PATH=../src$(PATH_SEPARATOR)$$PATH

check-local: $(SHELL) $(top_srcdir)/tests/test.sh

clean-local:
	rm -rf $(top_builddir)/tests/outputs

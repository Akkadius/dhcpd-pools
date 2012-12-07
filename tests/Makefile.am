EXTRA_DIST = \
	bootp \
	full-json \
	confs \
	expected \
	leading0 \
	leases \
	same-twice \
	simple

TESTS = \
	bootp \
	full-json \
	leading0 \
	same-twice \
	simple \
	v6

check-local: $(SHELL) $(top_srcdir)/tests/test.sh

clean-local:
	rm -rf $(top_builddir)/tests/outputs

#!/bin/sh
#
# Minimal regression test suite.

IAM=$(basename $0)

if [ ! -d tests/outputs ]; then
	mkdir tests/outputs
fi

dhcpd-pools -c $top_srcdir/tests/confs/$IAM \
	    -l $top_srcdir/tests/leases/$IAM -o tests/outputs/$IAM
diff -u $top_srcdir/tests/expected/$IAM tests/outputs/$IAM
exit $?

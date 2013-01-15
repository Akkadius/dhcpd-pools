#!/bin/sh
#
# Minimal regression test suite.

IAM=$(basename $0)

if [ ! -d outputs ]; then
	mkdir outputs
fi

dhcpd-pools -c $top_srcdir/tests/confs/$IAM \
	    -l $top_srcdir/tests/leases/$IAM -o outputs/$IAM
diff -u $top_srcdir/tests/expected/$IAM outputs/$IAM
exit $?

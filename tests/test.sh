#!/bin/sh
#
# Minimal regression test suite.

TEST_TOPDIR=$(cd $(dirname $0) && pwd)
IAM=$(basename $0)
DHCPD_POOLS=$(readlink -f $TEST_TOPDIR/../src/dhcpd-pools)

if [ ! -d outputs ]; then
	mkdir outputs
fi

$DHCPD_POOLS -c confs/$IAM -l leases/$IAM -o outputs/$IAM
diff -q expected/$IAM outputs/$IAM >/dev/null
exit $?

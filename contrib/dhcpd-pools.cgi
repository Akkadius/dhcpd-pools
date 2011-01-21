#!/bin/sh
#
# Simple CGI for dhcpd-pools.

echo Content-type: text/html
echo

# To make lease table more fancy use CSS definition something
# like this in your style.css file.
#
# TABLE.dhcpd-pools {
#    border-style : groove;
#    margin-left : 2px;
#    foo : bar;
# }
#
# http://www.w3.org/TR/REC-CSS2/tables.html
#
# And uncomment this line.
#
#echo <link type="text/css" rel="stylesheet" href="/style.css" />

echo "<html>"
echo "<body>"
echo "<p>This was situation at "
date
echo "</p>"

/usr/local/bin/dhcpd-pools --format html

echo "</body>"
echo "</html>"

# EOF

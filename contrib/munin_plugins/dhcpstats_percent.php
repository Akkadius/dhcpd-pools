#!/usr/bin/env php
<?php
error_reporting(E_ALL);
ini_set("display_errors", true);

//$xml = file_get_contents("test.xml");
$ret="";
$xml = exec("dhcpd-pools --format x --sort n", $ret);
$xml = join($ret);

$xml = new SimpleXMLElement($xml);


if ($argc>1 && $argv[1]=="autoconf") {
    echo "yes\n";
    return;
} elseif ($argc>1 && $argv[1]=="config") {
    $xml2 = $xml->xpath("subnet");
    echo "graph_title dhcp usage (in percent)\n";
    echo "graph_args --upper-limit 100 -l 0 --rigid\n";
    echo "graph_vlabel %\n";
    echo "graph_scale no\n";
    echo "graph_category network\n";
    foreach ($xml2 as $xml3) {
        $location = (string) $xml3->location;
	$range    = (string) $xml3->range;
        echo "$location.label $location ($range)\n";
        echo "$location.warning 75\n";
        echo "$location.critical 90\n";
    }
} else {
    $xml2 = $xml->xpath("shared-network");
    foreach ($xml2 as $xml3) {
        $location = (string) $xml3->location;
        $used     = (int) $xml3->used;
        $defined  = (int) $xml3->defined;
        // keep 1 digit after decimal point
        $pourcent = ceil($used*1000/$defined)/10;
        echo "$location.value $pourcent\n";
    }
}


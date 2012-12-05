#!/usr/bin/env php
<?php
error_reporting(E_ALL);
ini_set("display_errors", true);

$ret="";
$xml = exec("dhcpd-pools --format x --sort n", $ret);
$xml = join($ret);

$xml = new SimpleXMLElement($xml);


if ($argc>1 && $argv[1]=="autoconf") {
    echo "yes\n";
    return;
} elseif ($argc>1 && $argv[1]=="config") {
    $xml2 = $xml->xpath("subnet");
    echo "graph_title dhcp usage (number of machines)\n";
    echo "graph_vlabel nb of machines\n";
    echo "graph_scale no\n";
    echo "graph_category network\n";
    foreach ($xml2 as $xml3) {
        $location = (string) $xml3->location;
	$range    = (string) $xml3->range;
        echo "$location.label $location ($range)\n";
    }
} else {
    $xml2 = $xml->xpath("shared-network");
    foreach ($xml2 as $xml3) {
        $location = (string) $xml3->location;
        $used     = (int) $xml3->used;
        $defined  = (int) $xml3->defined;
        $pourcent = ceil($used*10000/$defined)/100;
        echo "$location.value $used\n";
    }
}


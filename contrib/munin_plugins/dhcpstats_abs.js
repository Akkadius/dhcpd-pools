#!/usr/bin/env node

var command = "dhcpd-pools"
var timeout = 5000;     // (ms) if the command take longer, then kill it

var execFile = require('child_process').execFile;
var echo     = require('util').print;

var arg = process.argv[2];

if ("autoconf" === arg) {
    echo("yes\n");
    return 0;
}

execFile(command, [ "--format=j", "--sort=n" ], { timeout: timeout }, function(error, stdout, stderr) {
    if (error !== null) {
        console.log('exec error: ' + error);
        return 1;
    }

    stdout = JSON.parse(stdout);

    if ("config" === arg) {
        echo("graph_title dhcp usage (number of machines)\n");
        echo("graph_vlabel nb of machines\n");
        echo("graph_scale no\n");
        echo("graph_category network\n");

        stdout["subnets"].forEach(function(subnet) {
            var location = subnet["location"];
            var range    = subnet["range"];
            echo(location + '.label ' + location + ' (' + range + ')\n');
        });
    } else {
        stdout["shared-networks"].forEach(function(network) {
            var location = network["location"];
            var used     = network["used"];
            var defined  = network["defined"];
            echo(location + '.value ' + used + '\n');
        });
    }

});

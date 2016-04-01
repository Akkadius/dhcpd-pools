# This is ISC dhcpd pool statistics script.
#
# The script is first version which was used to test that
# analysis algorithm is sane. This script is no longer
# maintained, and can be considered as historic reference.
#
# Licensed under the Open Software License version 1.1
# http://opensource.org/licenses/osl.php
#
# Sami Kerola <sami.kerola@teliasonera.com>
#
# Latest version is available from http://www.iki.fi/kerolasa/dhcp/
# This is version 1.4

BEGIN {
    # Do you want to see statistics per pool? 1 = yes, 0 = no.
    PrintRanges = 1

    leasefile="/opt/dhcp/db/dhcpd.leases"
    #conffile="/opt/dhcp/etc/dhcpd.conf"

    # External commands.
    uniq = "sort -nu > afterlp"
    sort = "sort -nu -k 2 > aftercp"
    uniqstdout = "sort -u"

    # Do the trick.
    main()
}

# Convert ip to decimal number.
function ip2num(addr) {
    split(addr, z, ".");
    numb = z[4]+256*(z[3]+256*(z[2]+256*z[1]))
    return numb
}

# Print decimal ip in dotted format.
function printip(number) {
    a = int(number/16777216)
    number = (number-(a*16777216))
    b = int(number/65536)
    number = int((number-(b*65536)))
    c = int(number/256)
    number = (number-(c*256))
    ipstring = sprintf("%d.%d.%d.%d", a, b, c, number)
    printf "%16s", ipstring
}

# Parse dhcpd lease file.
function parseleases() {
    if (system("test -r " leasefile) != 0) {
        print "File " leasefile " is not readable, exiting."
        exit 1
    }
    while (getline < leasefile) {
        if ($0 ~ /^lease/) {
            A = ip2num($2)
        }
        if ($0 ~ /^[    ]*binding state active/) {
            printf "%d\n", A | uniq
        }
    }
    close(leasefile)
    close(uniq)
}

# Parse dhcpd configuration file.
function parseconf() {
    while (getline) {  
        if ($0 ~ /^[    ]*shared-network/) {
            sharnet = $2
        }
        if ($0 ~ /^[    ]*range/) {
            printf "%s %d %d\n", sharnet, ip2num($2), ip2num($3) | sort
        }
    }
    close(sort)
}

# Join preparsed tmp files and output pool status.
function join() {
    # Get shared networks and pool ranges.
    i = 1
    while (getline < "aftercp") {
        split($0, j, " ")
        cnf[i,1] = j[1]
        cnf[i,2] = j[2]
        if (j[3] == 0) {
            j[3] = j[2]
        }
        cnf[i,3] = j[3]
        shnet[j[1],1] += j[3]-j[2]+1
        shnet[j[1],2] = 0
        allofall[1] += j[3]-j[2]+1
        i++
    }
    close("aftercp")
    i = 1
    allofall[2] = k = 0
    thisnet = ""
    # Count IPs in pools.
    if (PrintRanges) {
        # Print header.
        print "shared-network            1st in pool    last in pool   max   cur    usage%"
    }
    k = 0;
    while (getline < "afterlp") {
        ip = $0
          allofall[2]++
        # Ip out of the range, pool is changing.
        while (ip > cnf[i,3]) {
          if (PrintRanges) {
              printf "%-20s ", cnf[i,1]
              printip(cnf[i,2])
              printip(cnf[i,3])
              printf " %5d %5d %9.2f\n", cnf[i,3]-cnf[i,2]+1, k, k/(cnf[i,3]-cnf[i,2]+1)
              k = 0;
            }
            if (cnf[i,1] != thisnet) {
                # Shared network changed.
                thisnet = cnf[i,1]
            }
            i++
        }
        if (ip > cnf[i,2] && ip < cnf[i,3]) {
            # Add ip.
            shnet[cnf[i,1],2]++
        }
        k++;
    }
 
    # Print pool statistics.


    if (PrintRanges) {
        printf "%-20s ", cnf[i,1]
        printip(cnf[i,2])
        printip(cnf[i,3])
        printf " %5d %5d %9.2f\n", cnf[i,3]-cnf[i,2]+1, k, k/(cnf[i,3]-cnf[i,2]+1)
    }
    close("afterlp")
    # Print header and shared network statistics.
    print "#shared-network            max      cur   usage%"
    for (items in shnet) {
        x = substr(items, 1, length(items)-2)
        printf "%-20s %8d %8d %8.4f\n", x, shnet[x,1], shnet[x,2], shnet[x,2]/shnet[x,1] | uniqstdout
    }
    close(uniqstdout)
    print "#--------------"
    printf "%-20s %8d %8d %8.4f\n", "all_pools", allofall[1], allofall[2], allofall[2]/allofall[1]
}

# Remove temporary files.
function cleanup() {
    system("rm -f aftercp afterlp")
}

# Main.
function main() {
    system("touch afterlp aftercp")
    parseleases()
    parseconf()
    join()
    cleanup()
}

# EOF

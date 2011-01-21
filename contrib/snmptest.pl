#!/usr/bin/perl

# <Note from="Sami Kerola">
#    This script is anonymous user contribution (due legal
#    department did not let the person the get credit because
#    they are afraid open source contributions). You know who you
#    are, and I am graceful to get this to be part of the
#    package.
# </Note>

# 1) modify snmpd.conf and add the following line.  Note that the
#    last field must match the path/name of the script:
# 
#    pass_persist .1.3.6.1.4.1.2021.250.255 /root/snmptest.pl
# 
#    This assumes you've already configured snmpd, i.e. community
#    strings, etc.
# 
# 2) Install the following script.  Note that if you changed the
#    OID string above, then you'll need to change the value for
#    $root in this file, too. Be sure that $datafile is set to
#    the location of the .CSV output from dhcpd-pools. NOTE: if
#    you set $dbg to 1 then output will be generated in /tmp.

use strict;

# Version info:
my $SNMPver = "snmp1.0";
my $DHCPver = "dhcp1.0";
my $VERSION = "$SNMPver/$DHCPver";
#
# set $dbg to a non-zero value to get debug output in /tmp
#
my $dbg=0;

#
# Here's an example entry for snmpd.conf:
#    pass_persist .1.3.6.1.4.1.2021.250.255 /path/to/this/script.pl
# so in this case $root should be '.1.3.6.1.4.1.2021.250.255'
#
# $root.1.x : shared network name for index x
# $root.2.x : IP range name for index x
# $root.3.x : Range Contained-In for index x
# $root.4.x : Shared Net Stats for index x
# $root.5.x : IP range stats for index x
#
# $root.4.1.x : 'max' value for shared network at index=x
# $root.4.2.x : 'cur' value for shared network at index=x
# $root.4.3.x : 'tou' value for shared network at index=x
#
# $root.5.1.x : 'max' value for IP range at index=x
# $root.5.2.x : 'cur' value for IP range at index=x
# $root.5.3.x : 'tou' value for IP range at index=x

#
# $root must match pass_persis entry in snmpd.conf
#
my $root='.1.3.6.1.4.1.2021.250.255';

#
# $datafile is the location of the .CSV file created by dhcpd-pools.
# NOTE: It is assumed that this file is routinely updated, i.e. at least
# every 5 minutes, by some other process, i.e. cron.
#
my $datafile='/var/lib/dhcp/dhcpstats.csv';

#
# $cachetime determines how long we can cache parse data from $datafile. If
# more than $cachetime has elapsed we'll re-read $datafile, otherwise we'll
# just send back the last-parsed data
#
# Generally 60 seconds seems pretty reasonable, but any positive value is
# permitted.
#
my $cachetime = 60;

#
# global variables for DHCP
#
my %SharedNetwork;
my %RangeName;
my %RangeContained;
my @SNNindex;
my @IPRNindex;

#
# global variables for SNMP
#
my @validoidlist;

########## BEGIN UNIQUE-CODE FOR YOUR SNMP-QUERY NEEDS
#
# ParseDataFile does just that, and stores info in %SharedNetwork for later
# processing.
#
# The routine ParseDataFile is calls at invocation to initialize all
# datasets and also periodically (based on $cachetime).
#
my @where = qw( unknown Ranges SharedNets SumData );
sub ParseDataFile () {
   print DBG "ParseDataFile\n" if $dbg;
   my $where = 0; # location in file unknown
   # used to generate ifIndex-like values for each Shared network
   my $shareindex = 1;
   # used to generate ifIndex-like values for each Shared network
   my $rangeindex = 1;

   undef %SharedNetwork;
   undef %RangeName;
   undef %RangeContained;
   undef @SNNindex;
   undef @IPRNindex;
   @validoidlist = ();

   open(IN, $datafile) || return undef;
   while (<IN>) {
      chomp;
      print DBG "...Read: $_\n" if $dbg;
      next if /^\s*$/;   # skip blank lines
      next if /^"shared net name"/;    # skip titles
      next if /^"name","max","cur"/;   # skip titles
      if (/^"Ranges/) { $where = 1; next; }
      if (/^"Shared/) { $where = 2; next; }
      if (/^"Sum of/) { $where = 3; next; }
      print DBG "...Found data for @where[$where]: $_\n" if $dbg;
      #
      # "Ranges:"
      #  0                 1       2        3     4     5   6       7     8
      # "shared net name","1stIP","lastIP","max","cur","%","touch","t+c","t+c %"
      #
      # "Shared networks:" and "Sum of all ranges:"
      #  0      1     2     3   4       5     6
      # "name","max","cur","%","touch","t+c","t+c %"
      #
      my @data = split /,/;
      my ($nam, $max, $cur, $tou, $fir); 
      ($nam = @data[0]) =~ s/"//g;                  # shared net name

      ($fir = @data[1]) =~ s/"//g if ($where == 1); # Range ID (1st IP)

      ($max = @data[3]) =~ s/"//g if ($where == 1); # max # of IPs
      ($max = @data[1]) =~ s/"//g if ($where >  1); # max # of IPs

      ($cur = @data[4]) =~ s/"//g if ($where == 1); # cur # of IPs
      ($cur = @data[2]) =~ s/"//g if ($where >  1); # cur # of IPs

      ($tou = @data[6]) =~ s/"//g if ($where == 1); # touched IP's
      ($tou = @data[4]) =~ s/"//g if ($where >  1); # touched IP's


      print DBG "...idx sh/rg=$shareindex/$rangeindex : nam=$nam : max=$max : cur=$cur : tou=$tou\n" if $dbg;

      #
      # Summary data for each IP range
      #
      if ($where == 1) { # Store IP range data
         if (defined($RangeName{$fir})) {
            print DBG "...WARNING: duplicate name '$fir'\n" if $dbg
         }
         $IPRNindex[$rangeindex] = $fir;
         $RangeName{$fir} = pack("LLLL", $rangeindex, $max, $cur, $tou);
         $RangeContained{$fir} = $nam;
         push @validoidlist, "$root.2.$rangeindex";    # IP range name
         push @validoidlist, "$root.3.$rangeindex";    # Contained In
         push @validoidlist, "$root.5.1.$rangeindex";  # range stats
         push @validoidlist, "$root.5.2.$rangeindex";  # range stats
         push @validoidlist, "$root.5.3.$rangeindex";  # range stats
         $rangeindex++;
      }

      #
      # Summary data for each Shared network
      #
      if ($where == 2) { # Store Shared network summary data
         if (defined($SharedNetwork{$nam})) {
            print DBG "...WARNING: duplicate name '$nam'\n" if $dbg
         }
         $SNNindex[$shareindex] = $nam;
         $SharedNetwork{$nam} = pack("LLLL", $shareindex, $max, $cur, $tou);
         push @validoidlist, "$root.1.$shareindex";    # shared name
         push @validoidlist, "$root.4.1.$shareindex";  # shared stats
         push @validoidlist, "$root.4.2.$shareindex";  # shared stats
         push @validoidlist, "$root.4.3.$shareindex";  # shared stats
         $shareindex++;
      }

      #
      # Summary data for everything
      #
      if ($where == 3) { # Store "All" network summary data
         print DBG "...We don't store 'All' info yet!\n" if $dbg;
      }
   }
   close IN;
   if ($dbg) {
      foreach (sort @validoidlist) { print DBG "ValidOID: $_\n"; }
   }

   if ($dbg) {
      foreach (@IPRNindex) {
         print DBG "IP range   $_\n";
      }
      foreach (@SNNindex) {
         print DBG "Shared Net $_\n";
      }
   }
}

#############################################################################
#############################################################################
#############################################################################
# Forward Declarations
#
sub SendReturnNone ();
sub SendReturn($$$);
                 
#
# GetData gets data, but will leverage caching of data if last parse was
# over 60 seconds ago
#
my $lasttime = 0;
sub GetData ($) {
   my $oid = shift;

   (my $suboid = $oid) =~ s/$root//;

   #
   # If enough time has elapsed, go fetch fresh data, otherwise use cached
   # data
   #
   print DBG "GetData: Time is: ", time, " : oid $oid->$suboid\n" if $dbg;
   if ((time - $lasttime) > $cachetime) {
      ParseDataFile();
      $lasttime = time;
   }

   #
   # Quick sanity check of OID string:
   #
   if ( (!($oid =~ /^$root/)) ||
          ($suboid eq '')   ) { 
      SendReturnNone();
      return;
   }

   #
   # split the remaining OID pieces to analyse
   #
   my @oidlist = split(/\./, $suboid);

   ###################### EDIT THIS FOR YOUR APPLICATION ######################
   #
   # If we only get a single value, then barf (we need 2)
   #
   my $good = 0;
   $good = 1 if (($oidlist[1] eq '1') && ($#oidlist == 2)); # shared name
   $good = 1 if (($oidlist[1] eq '2') && ($#oidlist == 2)); # ip range name
   $good = 1 if (($oidlist[1] eq '3') && ($#oidlist == 2)); # contained in
   $good = 1 if (($oidlist[1] eq '4') && ($#oidlist == 3)); # shared-range stats
   $good = 1 if (($oidlist[1] eq '5') && ($#oidlist == 3)); # ip-range stats

   if (!$good) {
      print DBG "oidlistcount = $#oidlist and oidlist1 is ", $oidlist[1], "\n" if $dbg;
      SendReturnNone();
      return;
   }

   # $oidlist[1] = 1-5
   # $oidlist[2] = index for shared-name or range-name
   #            or datatype to return
   # $oidlist[3] = index for datatype
   #
   if ($oidlist[1] eq '1') {   # looking for name associated with shared net
      SendReturn($oid, 'string', $SNNindex[$oidlist[2]]);
      return;
   }

   if ($oidlist[1] eq '2') {   # looking for name associated with ip range
      print DBG "Responding with IPRNindex[$oidlist[2]]\n" if $dbg;
      SendReturn($oid, 'string', $IPRNindex[$oidlist[2]]);
      return;
   }

   if ($oidlist[1] eq '3') {   # looking for contained-in info
      if (defined($IPRNindex[$oidlist[2]])) { # valid subnet!
         my $fir = $IPRNindex[$oidlist[2]];
         my $con = $RangeContained{$fir};
         my @vals= unpack("LLLL", $SharedNetwork{$con});
         print DBG "fir=$fir : con=$con : vals=@vals : vals0=$vals[0]\n" if $dbg;
         SendReturn($oid, 'integer', @vals[0]);
         return;
      } else {
         SendReturnNone();
         return;
      }
   }

   if ($oidlist[1] eq '4') {
      if (defined($SNNindex[$oidlist[3]])) { # valid subnet!
         my $nam = $SNNindex[$oidlist[3]];
         my @vals= unpack("LLLL", $SharedNetwork{$nam});
         print DBG "nam=$nam, vals=@vals, oidlist[2]=$oidlist[2]\n" if $dbg;
         SendReturn($oid, 'integer', @vals[$oidlist[2]]);
         return;
      } else {
         print DBG "invalid subnet SNN $oidlist[3]\n" if $dbg;
         SendReturnNone();
         return;
      }
   }

   if ($oidlist[1] eq '5') {
      if (defined($IPRNindex[$oidlist[3]])) { # valid subnet!
         my $fir = $IPRNindex[$oidlist[3]];
         my @vals= unpack("LLLL", $RangeName{$fir});
         print DBG "fir=$fir, vals=@vals, oidlist[2]=", $oidlist[2], "\n" if $dbg;
         SendReturn($oid, 'integer', @vals[$oidlist[2]]);
         return;
      } else {
         print DBG "invalid subnet IPRN $oidlist[3]\n" if $dbg;
         SendReturnNone();
         return;
      }
   }
   ################## END EDIT THIS FOR YOUR APPLICATION ######################
}

#############################################################################
#############################################################################
#############################################################################
#
# From here down the routines should NEVER change
#
#
# SNMP-specific routines
#

{ ######## limit scope for some variable
   #
   # Compare looks at the OID validoid and userquery
   # and returns +1 if userquery > validoid
   # and returns -1 if userquery < validoid
   # and returns  0 if userquery = validoid
   #
   my @validoid;
   my @userquery;
   sub Compare () {
      my $i=1;
      while (($i <= $#validoid) && ($i <= $#userquery)) {
         # print DBG "Comparing $validoid[$i] vs $userquery[$i]\n" if $dbg;
         if ($userquery[$i] > $validoid[$i]) { return +1; }
         if ($userquery[$i] < $validoid[$i]) { return -1; }
         $i++;
      }
      $i--;
      my $returnval = 0;
      $returnval = +1 if (($i <  $#userquery) && ($i == $#validoid));
      $returnval = -1 if (($i == $#userquery) && ($i <  $#validoid));
      return $returnval;
   }

   #
   # FindNext looks through the list of validoid's trying to find the Next
   # oid after $oid (aka @userquery)
   #
   sub FindNext ($) {
      my $userqueryoid = shift;
      print DBG "FindNext($userqueryoid)\n" if $dbg;
      my $next = $validoidlist[0];

      @userquery = split (/\./, $userqueryoid);
      my $found = 0;
      foreach (sort @validoidlist) {
         $next = $_;
         print DBG "Comparing $userqueryoid vs. $_\n" if $dbg;
         @validoid = split (/\./, $_);
         my $x = Compare();
         if ($x < 0) {
            $found = 1;
            last;
         }
      }
      if (!$found) { return undef; }

      print DBG "Returning $next as next valid OID\n" if $dbg;
      return $next;
   }

} ######### end scope limit

sub SendReturnNone () {
   print "NONE\n";
   print DBG "Sent NONE\n" if $dbg;
}

sub SendReturn($$$) {
   printf "%s\n%s\n%s\n", shift, shift, shift;
}

my $line=0;
sub Get ($$) {
   my $cmd = shift;
   my $oid = shift;
   GetData($oid);
   $line = 0;
}

sub GetNext ($$) {
   my $cmd = shift;
   my $oid = shift;
   $oid = FindNext($oid);
   if (defined($oid)) {
      GetData($oid);
   } else {
      SendReturnNone();
   }
   $line = 0;
}

sub Set ($$$) {
   my $cmd = shift;
   my $oid = shift;
   my $tv  = shift;
   print "not-writable\n"; # we don't support snmpset
   print DBG "Sent not-writable\n" if $dbg;
   $line = 0;
}

sub Pong {
   print "PONG\n";
   print DBG "PONG Sent\n" if $dbg;
   $line = 0;
}

################################## START ##################################
#
# Main
#

select((select(STDOUT), $| = 1)[0]);

if ($dbg) {
   open(DBG, ">/tmp/snmp.dbg") || die ("Can't open debug!");
   select((select(DBG), $| = 1)[0]);
   print DBG "Version $VERSION\n";
}

# Initialize Data
ParseDataFile();

my $cmd;
my $oid;
my $tv;
while (<STDIN>) {
   $line++;
   chomp;
   s/ //g;
   tr/A-Z/a-z/;
   printf DBG "%3d '%s'\n", $line, $_ if $dbg;
   last if ($_ eq '');
   $cmd = $_ if ($line  == 1);
   $oid = $_ if ($line  == 2);
   $tv  = $_ if ($line  == 3);
   Pong()               if  ($cmd eq 'ping');
   Get($cmd, $oid)      if (($cmd eq 'get')     && ($line == 2));
   GetNext($cmd, $oid)  if (($cmd eq 'getnext') && ($line == 2));
   Set($cmd, $oid, $tv) if (($cmd eq 'set')     && ($line == 3));
}

print DBG "Clean Exit\n" if $dbg;
close DBG if $dbg;
exit 0;

### This is dhcpd-pools - ISC dhcpd lease status utility.

# Quick start (Debian & Ubuntu)

<pre>
sudo apt-get install -y unzip
sudo apt-get install -y libtool

git clone https://github.com/Akkadius/dhcpd-pools.git

cd /tmp
wget https://github.com/troydhanson/uthash/archive/master.zip
unzip master.zip

cd /tmp/dhcpd-pools
./bootstrap	# only when building git clone
./configure --with-uthash=/tmp/uthash-master/include
make -j4
make check
make install
</pre>

Dependencies to other projects.
* http://www.gnu.org/software/gnulib/
* http://uthash.sourceforge.net/
* https://getbootstrap.com/
* https://datatables.net/

Original credit:
http://dhcpd-pools.sourceforge.net/

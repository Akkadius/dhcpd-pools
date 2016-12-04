/*
 * The dhcpd-pools has BSD 2-clause license which also known as "Simplified
 * BSD License" or "FreeBSD License".
 *
 * Copyright 2006- Sami Kerola. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR AND CONTRIBUTORS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of Sami Kerola.
 */

/*! \file other.c
 * \brief Collection of various functions.
 */

#include <config.h>

#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "error.h"
#include "progname.h"
#include "quote.h"

#include "dhcpd-pools.h"
#include "defaults.h"

/*! \brief Set function pointers depending on IP version.
 * \param ip IP version.
 */
void set_ipv_functions(int version)
{
	switch (version) {

	case IPv4:
		config.ip_version = version;
		add_lease = add_lease_v4;
		copy_ipaddr = copy_ipaddr_v4;
		find_lease = find_lease_v4;
		get_range_size = get_range_size_v4;
		ipcomp = ipcomp_v4;
		leasecomp = leasecomp_v4;
		ntop_ipaddr = ntop_ipaddr_v4;
		parse_ipaddr = parse_ipaddr_v4;
		xstrstr = xstrstr_v4;
		break;

	case IPv6:
		config.ip_version = version;
		add_lease = add_lease_v6;
		copy_ipaddr = copy_ipaddr_v6;
		find_lease = find_lease_v6;
		get_range_size = get_range_size_v6;
		ipcomp = ipcomp_v6;
		leasecomp = leasecomp_v6;
		ntop_ipaddr = ntop_ipaddr_v6;
		parse_ipaddr = parse_ipaddr_v6;
		xstrstr = xstrstr_v6;
		break;

	case IPvUNKNOWN:
		config.ip_version = version;
		add_lease = add_lease_init;
		copy_ipaddr = copy_ipaddr_init;
		find_lease = find_lease_init;
		get_range_size = get_range_size_init;
		ipcomp = ipcomp_init;
		leasecomp = leasecomp_init;
		ntop_ipaddr = ntop_ipaddr_init;
		parse_ipaddr = parse_ipaddr_init;
		xstrstr = xstrstr_init;
		break;

	default:
		abort();

	}
	return;
}

/*! \brief Convert text string IP address from either IPv4 or IPv6 to an integer.
 * \param src An IP string in either format.
 * \param dst An union which will hold conversion result.
 * \return Was parsing successful.
 */
int parse_ipaddr_init(const char *restrict src, union ipaddr_t *restrict dst)
{
	struct in_addr addr;
	struct in6_addr addr6;

	if (inet_aton(src, &addr) == 1)
		set_ipv_functions(IPv4);
	else if (inet_pton(AF_INET6, src, &addr6) == 1)
		set_ipv_functions(IPv6);
	else
		return 0;
	return parse_ipaddr(src, dst);
}

int parse_ipaddr_v4(const char *restrict src, union ipaddr_t *restrict dst)
{
	int rv;
	struct in_addr addr;

	rv = inet_aton(src, &addr);
	dst->v4 = ntohl(addr.s_addr);
	return rv == 1;
}

int parse_ipaddr_v6(const char *restrict src, union ipaddr_t *restrict dst)
{
	int rv;
	struct in6_addr addr;

	rv = inet_pton(AF_INET6, src, &addr);
	memcpy(&dst->v6, addr.s6_addr, sizeof(addr.s6_addr));
	return rv == 1;
}

/*! \brief Copy IP address to union.
 *
 * \param dst Destination for a binary IP address.
 * \param src Sourse of an IP address. */
void copy_ipaddr_init(union ipaddr_t *restrict dst __attribute__ ((unused)),
		      const union ipaddr_t *restrict src __attribute__ ((unused)))
{
}

void copy_ipaddr_v4(union ipaddr_t *restrict dst, const union ipaddr_t *restrict src)
{
	dst->v4 = src->v4;
}

void copy_ipaddr_v6(union ipaddr_t *restrict dst, const union ipaddr_t *restrict src)
{
	memcpy(&dst->v6, &src->v6, sizeof(src->v6));
}

/*! \brief Convert an address to string. This function will convert the
 * IPv4 addresses to 123.45.65.78 format, and the IPv6 addresses to it's
 * native format depending on which version of the addressing is found to
 * be in use.
 *
 * \param ip Binary IP address.
 * \return Printable address.
 */
const char *ntop_ipaddr_init(const union ipaddr_t *ip __attribute__ ((unused)))
{
	static char buffer = '\0';

	return &buffer;
}

const char *ntop_ipaddr_v4(const union ipaddr_t *ip)
{
	static char buffer[sizeof("255.255.255.255")];
	struct in_addr addr;

	addr.s_addr = htonl(ip->v4);
	return inet_ntop(AF_INET, &addr, buffer, sizeof(buffer));
}

const char *ntop_ipaddr_v6(const union ipaddr_t *ip)
{
	static char buffer[sizeof("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255")];
	struct in6_addr addr;

	memcpy(addr.s6_addr, ip->v6, sizeof(addr.s6_addr));
	return inet_ntop(AF_INET6, &addr, buffer, sizeof(buffer));
}

/*! \brief Calculate how many addresses there are in a range.
 *
 * \param r Pointer to range structure, which has information about first
 * and last IP in the range.
 * \return Size of a range.
 */
double get_range_size_init(const struct range_t *r __attribute__ ((unused)))
{
	return 0;
}

double get_range_size_v4(const struct range_t *r)
{
	return r->last_ip.v4 - r->first_ip.v4 + 1;
}

double get_range_size_v6(const struct range_t *r)
{
	double size = 0;
	int i;

	/* When calculating the size of an IPv6 range overflow may occur.
	 * In that case only the last LONG_BIT bits are preserved, thus
	 * we just skip the first (16 - LONG_BIT) bits...  */
	for (i = 0; i < 16; i++) {
		size *= 256;
		size += (int)r->last_ip.v6[i] - (int)r->first_ip.v6[i];
	}
	return size + 1;
}

/*! \fn xstrstr_init(const char *restrict str)
 * \brief Determine if the dhcpd is in IPv4 or IPv6 mode. This function
 * may be needed when dhcpd.conf file has zero IP version hints.
 *
 * \param str A line from dhcpd.conf
 * \return prefix_t enum value
 */
int
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
    __attribute__ ((hot))
#endif
    xstrstr_init(const char *restrict str)
{
	if (memcmp("lease ", str, 6)) {
		set_ipv_functions(IPv4);
		return PREFIX_LEASE;
	} else if (memcmp("  iaaddr ", str, 9)) {
		set_ipv_functions(IPv6);
		return PREFIX_LEASE;
	}
	return NUM_OF_PREFIX;
}

/*! \fn xstrstr_v4(const char *restrict str)
 * \brief parse lease file in IPv4 mode
 *
 * \param str A line from dhcpd.conf
 * \return prefix_t enum value
 */
int
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
    __attribute__ ((hot))
#endif
    xstrstr_v4(const char *restrict str)
{
	size_t len;

	if (str[2] == 'b' || str[2] == 'h')
		len = strlen(str);
	else
		len = 0;
	if (15 < len) {
		switch (str[16]) {
		case 'f':
			if (!memcmp("  binding state free;", str, 21))
				return PREFIX_BINDING_STATE_FREE;
			break;
		case 'a':
			if (!memcmp("  binding state active;", str, 23))
				return PREFIX_BINDING_STATE_ACTIVE;
			if (!memcmp("  binding state abandoned;", str, 25))
				return PREFIX_BINDING_STATE_ABANDONED;
			break;
		case 'e':
			if (!memcmp("  binding state expired;", str, 24))
				return PREFIX_BINDING_STATE_EXPIRED;
			break;
		case 'r':
			if (!memcmp("  binding state released;", str, 25))
				return PREFIX_BINDING_STATE_RELEASED;
			break;
		case 'b':
			if (!memcmp("  binding state backup;", str, 23))
				return PREFIX_BINDING_STATE_BACKUP;
			break;
		case 'n':
			if (!memcmp("  hardware ethernet", str, 19))
				return PREFIX_HARDWARE_ETHERNET;
			break;
		}
	}
	if (!memcmp("lease ", str, 6))
		return PREFIX_LEASE;
	return NUM_OF_PREFIX;
}

/*! \fn xstrstr_v4(const char *restrict str)
 * \brief parse lease file in IPv6 mode
 *
 * \param str A line from dhcpd.conf
 * \return prefix_t enum value
 */
int
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
    __attribute__ ((hot))
#endif
    xstrstr_v6(const char *restrict str)
{
	size_t len;

	if (str[4] == 'b' || str[2] == 'h')
		len = strlen(str);
	else
		len = 0;
	if (17 < len) {
		switch (str[18]) {
		case 'f':
			if (!memcmp("    binding state free;", str, 23))
				return PREFIX_BINDING_STATE_FREE;
			break;
		case 'a':
			if (!memcmp("    binding state active;", str, 25))
				return PREFIX_BINDING_STATE_ACTIVE;
			if (!memcmp("    binding state abandoned;", str, 27))
				return PREFIX_BINDING_STATE_ABANDONED;
			break;
		case 'e':
			if (!memcmp("    binding state expired;", str, 26))
				return PREFIX_BINDING_STATE_EXPIRED;
			break;
		case 'r':
			if (!memcmp("    binding state released;", str, 27))
				return PREFIX_BINDING_STATE_RELEASED;
			break;
		case 'b':
			if (!memcmp("    binding state backup;", str, 25))
				return PREFIX_BINDING_STATE_BACKUP;
			break;
		case 'n':
			if (!memcmp("  hardware ethernet", str, 19))
				return PREFIX_HARDWARE_ETHERNET;
			break;
		}
	}
	if (!memcmp("  iaaddr ", str, 9))
		return PREFIX_LEASE;
	return NUM_OF_PREFIX;
}

/*! \brief Return a double floating point value.
 *
 * \param str String to be converted to a double.
 * \param errmesg Exit error message if conversion fails.
 * \return Binary result of string to double conversion.
 */
double strtod_or_err(const char *restrict str, const char *restrict errmesg)
{
	double num;
	char *end = NULL;

	if (str == NULL || *str == '\0')
		goto err;
	errno = 0;
	num = strtod(str, &end);
	if (errno || str == end || (end && *end))
		goto err;
	return num;
 err:
	error(EXIT_FAILURE, errno, "%s: %s", errmesg, quote(str));
	return 0;
}

/*! \brief Reverse range.
 * Used before output, if a caller has requested reverse sorting.
 * FIXME: The temporary memory area handling should be internal to this
 * function, not a parameter.
 *
 * \param flip_me The range that needs to be inverted.
 * \param tmp_ranges Temporary memory area for the flip. */
void flip_ranges(struct range_t *restrict flip_me, struct range_t *restrict tmp_ranges)
{
	unsigned int i = num_ranges - 1, j;

	for (j = 0; j < num_ranges; j++, i--)
		*(tmp_ranges + j) = *(flip_me + i);
	memcpy(flip_me, tmp_ranges, num_ranges * sizeof(struct range_t));
}

/*! \brief Free memory, flush buffers etc. */
void clean_up(void)
{
	struct output_sort *cur, *next;

	/* Just in case there something in buffers */
	if (fflush(NULL))
		error(EXIT_FAILURE, errno, "clean_up: fflush");
	free(config.dhcpdconf_file);
	free(config.dhcpdlease_file);
	free(config.output_file);
	free(ranges);
	delete_all_leases();
	if (shared_networks) {
		unsigned int i;

		num_shared_networks++;
		for (i = 0; i < num_shared_networks; i++)
			free((shared_networks + i)->name);
		free(shared_networks);
	}
	for (cur = config.sorts; cur; cur = next) {
		next = cur->next;
		free(cur);
	}
}

/*! \brief A version printing. */
void __attribute__ ((__noreturn__)) print_version(void)
{
	fprintf(stdout, "%s\n"
		"Original design by Sami Kerola.\n"
		"XML support by Dominic Germain, Sogetel inc.\n"
		"IPv6 support by Cheer Xiao.\n\n"
		"The software has FreeBSD License.\n", PACKAGE_STRING);
	exit(EXIT_SUCCESS);
}

/*! \brief Command line help screen. */
void __attribute__ ((__noreturn__)) usage(int status)
{
	FILE *out = status == EXIT_SUCCESS ? stdout : stderr;

	fprintf(out,	"Usage: %s [OPTIONS]\n", program_name);
	fputs(		"\n", out);
	fputs(		"This is ISC dhcpd pools usage analyzer.\n", out);
	fputs(		"\n", out);
	fputs(		"  -c, --config=FILE      path to the dhcpd.conf file\n", out);
	fputs(		"  -l, --leases=FILE      path to the dhcpd.leases file\n", out);
	fputs(		"  -f, --format=[thHcxXjJ] output format\n", out);
	fputs(		"                           t for text\n", out);
	fputs(		"                           H for full html page\n", out);
	fputs(		"                           x for xml\n", out);
	fputs(		"                           X for xml with active lease details\n", out);
	fputs(		"                           j for json\n", out);
	fputs(		"                           J for json with active lease details\n", out);
	fputs(		"                           c for comma separated values\n", out);
	fputs(		"  -s, --sort=[nimcptTe]  sort ranges by\n", out);
	fputs(		"                           n name\n", out);
	fputs(		"                           i IP\n", out);
	fputs(		"                           m maximum\n", out);
	fputs(		"                           c current\n", out);
	fputs(		"                           p percent\n", out);
	fputs(		"                           t touched\n", out);
	fputs(		"                           T t+c\n", out);
	fputs(		"                           e t+c perc\n", out);
	fputs(		"  -r, --reverse          reverse order sort\n", out);
	fputs(		"  -o, --output=FILE      output into a file\n", out);
	fputs(		"  -L, --limit=NR         output limit mask 77 - 00\n", out);
	fputs(		"      --warning=PERC     set warning alarming limit\n", out);
	fputs(		"      --critical=PERC    set critical alarming limit\n", out);
	fputs(		"      --warn-count=NR    a number of free leases before warning raised\n", out);
	fputs(		"      --crit-count=NR    a number of free leases before critical raised\n", out);
	fputs(		"      --minsize=size     disable alarms for small ranges and shared-nets\n", out);
	fputs(		"      --snet-alarms      suppress range alarms that are part of a shared-net\n", out);
	fputs(		"  -p, --perfdata         print additional perfdata in alarming mode\n", out);
	fputs(		"  -A, --all-as-shared    treat single subnets as shared-network with CIDR as their name\n", out);
	fputs(		"  -v, --version          output version information and exit\n", out);
	fputs(		"  -h, --help             display this help and exit\n", out);
	fputs(		"\n", out);
	fprintf(out,	"Report bugs to <%s>\n", PACKAGE_BUGREPORT);
	fprintf(out,	"Homepage: %s\n", PACKAGE_URL);

	exit(status);
}

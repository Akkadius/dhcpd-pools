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

#ifndef DHCPD_POOLS_H
# define DHCPD_POOLS_H 1

# include <config.h>
# include <arpa/inet.h>
# include <stddef.h>
# include <stdio.h>
# include <string.h>
# include <uthash.h>

# ifdef HAVE_BUILTIN_EXPECT
#  define likely(x)	__builtin_expect(!!(x), 1)
#  define unlikely(x)	__builtin_expect(!!(x), 0)
# else
#  define likely(x)	(x)
#  define unlikely(x)	(x)
# endif

/* Structures and unions */
union ipaddr_t {
	uint32_t v4;
	unsigned char v6[16];
};
enum dhcp_version {
	VERSION_4,
	VERSION_6,
	VERSION_UNKNOWN,
};
enum prefix_t {
	PREFIX_LEASE,
	PREFIX_BINDING_STATE_FREE,
	PREFIX_BINDING_STATE_ACTIVE,
	PREFIX_BINDING_STATE_BACKUP,
	PREFIX_HARDWARE_ETHERNET,
	NUM_OF_PREFIX
};

struct configuration_t {
	char dhcpv6;
	char *dhcpdconf_file;
	char *dhcpdlease_file;
	char output_format[2];
	char sort[6];
	int reverse_order;
	char *output_file;
	int output_limit[2];
	double warning;
	double critical;
};
struct shared_network_t {
	char *name;
	unsigned long int available;
	unsigned long int used;
	unsigned long int touched;
	unsigned long int backups;
};
struct range_t {
	struct shared_network_t *shared_net;
	union ipaddr_t first_ip;
	union ipaddr_t last_ip;
	unsigned long int count;
	unsigned long int touched;
	unsigned long int backups;
};
enum ltype {
	ACTIVE,
	FREE,
	BACKUP
};
struct leases_t {
	union ipaddr_t ip;	/* ip as key */
	enum ltype type;
	char *ethernet;
	UT_hash_handle hh;
};

/* Global variables */
const char *prefixes[2][NUM_OF_PREFIX];
int prefix_length[2][NUM_OF_PREFIX];
struct configuration_t config;
enum dhcp_version dhcp_version;
static int const output_limit_bit_1 = 1;
static int const output_limit_bit_2 = 2;
static int const output_limit_bit_3 = 4;
unsigned int fullhtml;
struct shared_network_t *shared_networks;
unsigned int num_shared_networks;
struct range_t *ranges;
unsigned int num_ranges;
struct leases_t *leases;
unsigned long int num_leases;
unsigned long int num_touches;
unsigned long int num_backups;

/* Function prototypes */
int prepare_memory(void);
int parse_leases(void);
void parse_config(int, const char *__restrict,
		  struct shared_network_t *__restrict)
    __attribute__ ((nonnull(2, 3)));
void nth_field(char *__restrict dest, const char *__restrict src)
    __attribute__ ((nonnull(1, 2)))
# if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
    __attribute__ ((__hot__))
# endif
    ;
int prepare_data(void);
int do_counting(void);
void flip_ranges(struct range_t *__restrict ranges,
		 struct range_t *__restrict tmp_ranges)
    __attribute__ ((nonnull(1, 2)));
/* support functions */
int parse_ipaddr(const char *restrict src, union ipaddr_t *restrict dst);
void copy_ipaddr(union ipaddr_t *restrict dst,
		 const union ipaddr_t *restrict src);
const char *ntop_ipaddr(const union ipaddr_t *ip);
unsigned long get_range_size(const struct range_t *r);
int xstrstr(const char *__restrict a, const char *__restrict b, int len)
    __attribute__ ((nonnull(1, 2)))
# if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
    __attribute__ ((__hot__))
# endif
    ;
double strtod_or_err(const char *__restrict str,
		     const char *__restrict errmesg);
void print_version(void) __attribute__ ((noreturn));
void usage(int status) __attribute__ ((noreturn));
/* qsort required functions... */
/* ...for ranges and... */
int intcomp(const void *__restrict x, const void *__restrict y)
    __attribute__ ((nonnull(1, 2)));
int ipcomp(const union ipaddr_t *restrict a, const union ipaddr_t *restrict b);
int leasecomp(const void *restrict a, const void *restrict b);
int rangecomp(const void *__restrict r1, const void *__restrict r2)
    __attribute__ ((nonnull(1, 2)));
/* sort function pointer and functions */
int sort_name(void);
unsigned long int (*returner) (struct range_t r);
unsigned long int ret_ip(struct range_t r);
unsigned long int ret_cur(struct range_t r);
unsigned long int ret_max(struct range_t r);
unsigned long int ret_percent(struct range_t r);
unsigned long int ret_touched(struct range_t r);
unsigned long int ret_tc(struct range_t r);
unsigned long int ret_tcperc(struct range_t r);
void field_selector(char c);
int get_order(struct range_t *__restrict left, struct range_t *__restrict right)
    __attribute__ ((nonnull(1, 2)));
void mergesort_ranges(struct range_t *__restrict orig, int size,
		      struct range_t *__restrict temp)
    __attribute__ ((nonnull(1, 3)));
/* output function pointer and functions */
int (*output_analysis) (void);
int output_txt(void);
int output_html(void);
int output_xml(void);
int output_json(void);
int output_csv(void);
int output_alarming(void);
/* Memory release, file closing etc */
void clean_up(void);
/* Hash functions */
void add_lease(union ipaddr_t *ip, enum ltype type);
struct leases_t *find_lease(union ipaddr_t *ip);
void delete_lease(struct leases_t *lease);
void delete_all_leases();

#endif				/* DHCPD_POOLS_H */

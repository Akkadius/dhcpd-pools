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

/*! \file dhcpd-pools.h
 * \brief Global definitions of structures, enums, and function prototypes.
 * FIXME: The file has too many global variables. Most of them should be
 * removed, if not all.
 */

#ifndef DHCPD_POOLS_H
# define DHCPD_POOLS_H 1

# include <config.h>
# include <arpa/inet.h>
# include <stdbool.h>
# include <stddef.h>
# include <stdio.h>
# include <string.h>
# include <uthash.h>

/*! \def likely(x)
 * \brief Symbolic call to __builtin_expect'ed branch.
 */
/*! \def unlikely(x)
 * \brief Symbolic call to not-__builtin_expect'ed branch.
 */
# ifdef HAVE_BUILTIN_EXPECT
#  define likely(x)	__builtin_expect(!!(x), 1)
#  define unlikely(x)	__builtin_expect(!!(x), 0)
# else
#  define likely(x)	(x)
#  define unlikely(x)	(x)
# endif

/*! \union ipaddr_t
 * \brief Memory space for a binary IP address saving. */
union ipaddr_t {
	uint32_t v4;
	unsigned char v6[16];
};
/*! \enum dhcp_version
 * \brief Indicator which IP version is in use.
 */
enum dhcp_version {
	VERSION_4,
	VERSION_6,
	VERSION_UNKNOWN
};
/*! \enum prefix_t
 * \brief Enumeration of interesting data in dhcpd.leases file, that has
 * to be further examined, and saved.
 */
enum prefix_t {
	PREFIX_LEASE,
	PREFIX_BINDING_STATE_FREE,
	PREFIX_BINDING_STATE_ACTIVE,
	PREFIX_BINDING_STATE_BACKUP,
	PREFIX_HARDWARE_ETHERNET,
	NUM_OF_PREFIX
};
/*! \struct configuration_t
 * \brief Runtime configuration.
 */
struct configuration_t {
	char dhcpv6;
	enum dhcp_version dhcp_version;
	char *dhcpdconf_file;
	char *dhcpdlease_file;
	char output_format[2];
	bool fullhtml;
	char sort[6];
	bool reverse_order;
	char *output_file;
	int output_limit[2];
	bool backups_found;
	double warning;
	double critical;
};
/*! \struct shared_network_t
 * \brief Counters for an individual shared network.
 */
struct shared_network_t {
	char *name;
	double available;
	double used;
	double touched;
	double backups;
};
/*! \struct range_t
 * \brief Counters for an individual range.
 */
struct range_t {
	struct shared_network_t *shared_net;
	union ipaddr_t first_ip;
	union ipaddr_t last_ip;
	double count;
	double touched;
	double backups;
};
/*! \enum isc_conf_parser
 * \brief Configuration file parsing state flags.
 */
enum isc_conf_parser {
	ITS_NOTHING_INTERESTING,
	ITS_A_RANGE_FIRST_IP,
	ITS_A_RANGE_SECOND_IP,
	ITS_A_SHAREDNET,
	ITS_AN_INCLUCE
};
/*! \enum ltype
 * \brief Lease state types.
 */
enum ltype {
	ACTIVE,
	FREE,
	BACKUP
};
/*! \struct leases_t
 * \brief An individual lease. The leaases are hashed.
 */
struct leases_t {
	union ipaddr_t ip;	/* ip as key */
	enum ltype type;
	char *ethernet;
	UT_hash_handle hh;
};
/*! \enum limbits
 * \brief Output limit bits
 */
enum limbits {
	BIT1 = 0x1,
	BIT2 = 0x2,
	BIT3 = 0x4
};

/* Global variables */
/* \var prefix_length Length of each prefix.  */
int prefix_length[2][NUM_OF_PREFIX];
/* \var config Runtime configuration. */
struct configuration_t config;
/* \var shared_networks Pointer holding shared network count results. */
struct shared_network_t *shared_networks;
/* \var num_shared_networks Number of shared networks found. */
unsigned int num_shared_networks;
/* \var ranges Pointer holding range count results. */
struct range_t *ranges;
/* \var num_ranges Number of ranges found. */
unsigned int num_ranges;
/* \var leases Pointer holding all leases. */
struct leases_t *leases;
/*! \var RANGES Maximum number of ranges. */
unsigned int RANGES;

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
double get_range_size(const struct range_t *r);
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
int ipcomp(const union ipaddr_t *restrict a, const union ipaddr_t *restrict b);
int comp_cur(struct range_t *r1, struct range_t *r2);
int comp_double(double f1, double f2);
int comp_ip(struct range_t *r1, struct range_t *r2);
int comp_max(struct range_t *r1, struct range_t *r2);
int comp_percent(struct range_t *r1, struct range_t *r2);
int comp_tc(struct range_t *r1, struct range_t *r2);
int comp_tcperc(struct range_t *r1, struct range_t *r2);
int comp_touched(struct range_t *r1, struct range_t *r2);
int leasecomp(const void *restrict a, const void *restrict b);
int rangecomp(const void *__restrict r1, const void *__restrict r2)
    __attribute__ ((nonnull(1, 2)));
/* sort function pointer and functions */
typedef int (*comparer_t) (struct range_t *r1, struct range_t *r2);
comparer_t field_selector(char c);
double ret_percent(struct range_t r);
double ret_tc(struct range_t r);
double ret_tcperc(struct range_t r);
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
void delete_all_leases(void);

#endif				/* DHCPD_POOLS_H */

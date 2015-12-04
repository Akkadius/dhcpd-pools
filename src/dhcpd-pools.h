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

/* The __attribute__ feature is available in gcc versions 2.5 and later.
 * The attribute __pure__ was added in gcc 2.96.  */
# if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
#  define _DP_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define _DP_ATTRIBUTE_PURE	/* empty */
# endif

/* The __const__ attribute was added in gcc 2.95.  */
# if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
#  define _DP_ATTRIBUTE_CONST __attribute__ ((__const__))
# else
#  define _DP_ATTRIBUTE_CONST	/* empty */
# endif

/* The attribute __hot__ was added in gcc 4.3.  */
# if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
#  define _DP_ATTRIBUTE_HOT __attribute__ ((__hot__))
# else
#  define _DP_ATTRIBUTE_HOT	/* empty */
# endif

/*! \union ipaddr_t
 * \brief Memory space for a binary IP address saving. */
union ipaddr_t {
	uint32_t v4;
	unsigned char v6[16];
};
/*! \enum dhcp_version
 * \brief The IP version, IPv4 or IPv6, served by the dhcpd.
 */
enum dhcp_version {
	IPvUNKNOWN,
	IPv4,
	IPv6
};
/*! \enum prefix_t
 * \brief Enumeration of interesting data in dhcpd.leases file, that has
 * to be further examined, and saved.
 */
enum prefix_t {
	PREFIX_LEASE,
	PREFIX_BINDING_STATE_FREE,
	PREFIX_BINDING_STATE_ABANDONED,
	PREFIX_BINDING_STATE_EXPIRED,
	PREFIX_BINDING_STATE_RELEASED,
	PREFIX_BINDING_STATE_ACTIVE,
	PREFIX_BINDING_STATE_BACKUP,
	PREFIX_HARDWARE_ETHERNET,
	NUM_OF_PREFIX
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
 * \brief Output limit bits: R_BIT ranges, S_BIT shared networks, A_BIT all.
 */
enum limbits {
	R_BIT = (1 << 0),
	S_BIT = (1 << 1),
	A_BIT = (1 << 2)
};

/*! \def STATE_OK
 * \brief Nagios alarm exit values.
 */
# define STATE_OK 0
# define STATE_WARNING 1
# define STATE_CRITICAL 2

/*! \var comparer_t
 * \brief Function pointer holding sort algorithm.
 */
typedef int (*comparer_t) (struct range_t *r1, struct range_t *r2);

/*! \struct output_sort
 * \brief Linked list of sort functions.
 */
struct output_sort {
	comparer_t func;
	struct output_sort *next;
};
/*! \struct configuration_t
 * \brief Runtime configuration.
 */
struct configuration_t {
	char dhcpv6;
	enum dhcp_version ip_version;
	char *dhcpdconf_file;
	char *dhcpdlease_file;
	struct output_sort *sorts;
	char *output_file;
	double warning;
	double critical;
	double warn_count;
	double crit_count;
	double minsize;
	unsigned int
		reverse_order:1,
		backups_found:1,
		snet_alarms:1,
		print_mac_addreses:1,
		header_limit:3,
		number_limit:3;
};
/* Global variables */
/* \var prefix_length Length of each prefix.  */
extern int prefix_length[2][NUM_OF_PREFIX];
/* \var config Runtime configuration. */
extern struct configuration_t config;
/* \var shared_networks Pointer holding shared network count results. */
extern struct shared_network_t *shared_networks;
/* \var num_shared_networks Number of shared networks found. */
extern unsigned int num_shared_networks;
/* \var ranges Pointer holding range count results. */
extern struct range_t *ranges;
/* \var num_ranges Number of ranges found. */
extern unsigned int num_ranges;
/* \var leases Pointer holding all leases. */
extern struct leases_t *leases;
/*! \var RANGES Maximum number of ranges. */
extern unsigned int RANGES;

/* Function prototypes */
extern void prepare_memory(void);
extern void set_ipv_functions(int version);
extern int parse_leases(void);
extern void parse_config(int, const char *__restrict, struct shared_network_t *__restrict)
    __attribute__ ((nonnull(2, 3)));
extern void prepare_data(void);
extern void do_counting(void);
extern void flip_ranges(struct range_t *__restrict ranges, struct range_t *__restrict tmp_ranges)
    __attribute__ ((nonnull(1, 2)));
/* support functions */
extern int (*parse_ipaddr) (const char *restrict src, union ipaddr_t *restrict dst);
extern int parse_ipaddr_init(const char *restrict src,
			     union ipaddr_t *restrict dst) _DP_ATTRIBUTE_CONST;
extern int parse_ipaddr_v4(const char *restrict src, union ipaddr_t *restrict dst);
extern int parse_ipaddr_v6(const char *restrict src, union ipaddr_t *restrict dst);

extern void (*copy_ipaddr) (union ipaddr_t *restrict dst, const union ipaddr_t *restrict src);
extern void copy_ipaddr_init(union ipaddr_t *restrict dst,
			     const union ipaddr_t *restrict src) _DP_ATTRIBUTE_CONST;
extern void copy_ipaddr_v4(union ipaddr_t *restrict dst, const union ipaddr_t *restrict src);
extern void copy_ipaddr_v6(union ipaddr_t *restrict dst, const union ipaddr_t *restrict src);

extern const char *(*ntop_ipaddr) (const union ipaddr_t *ip);
extern const char *ntop_ipaddr_init(const union ipaddr_t *ip) _DP_ATTRIBUTE_CONST;
extern const char *ntop_ipaddr_v4(const union ipaddr_t *ip);
extern const char *ntop_ipaddr_v6(const union ipaddr_t *ip);

extern double (*get_range_size) (const struct range_t *r);
extern double get_range_size_init(const struct range_t *r) _DP_ATTRIBUTE_CONST;
extern double get_range_size_v4(const struct range_t *r) _DP_ATTRIBUTE_PURE;
extern double get_range_size_v6(const struct range_t *r) _DP_ATTRIBUTE_PURE;

extern int (*xstrstr) (const char *__restrict str);
extern int xstrstr_init(const char *__restrict str) _DP_ATTRIBUTE_CONST;
extern int xstrstr_v4(const char *__restrict str)
_DP_ATTRIBUTE_HOT _DP_ATTRIBUTE_PURE;
extern int xstrstr_v6(const char *__restrict str)
_DP_ATTRIBUTE_HOT _DP_ATTRIBUTE_PURE;

extern double strtod_or_err(const char *__restrict str, const char *__restrict errmesg);
extern void print_version(void) __attribute__ ((noreturn));
extern void usage(int status) __attribute__ ((noreturn));
/* qsort required functions... */
/* ...for ranges and... */
extern int (*ipcomp) (const union ipaddr_t *restrict a, const union ipaddr_t *restrict b);
extern int ipcomp_init(const union ipaddr_t *restrict a,
		       const union ipaddr_t *restrict b) _DP_ATTRIBUTE_CONST;
extern int ipcomp_v4(const union ipaddr_t *restrict a,
		     const union ipaddr_t *restrict b) _DP_ATTRIBUTE_PURE;
extern int ipcomp_v6(const union ipaddr_t *restrict a,
		     const union ipaddr_t *restrict b) _DP_ATTRIBUTE_PURE;

extern int (*leasecomp) (const struct leases_t *restrict a, const struct leases_t *restrict b);
extern int leasecomp_init(const struct leases_t *restrict a
			  __attribute__ ((unused)),
			  const struct leases_t *restrict b __attribute__ ((unused)));
extern int leasecomp_v4(const struct leases_t *restrict a, const struct leases_t *restrict b);
extern int leasecomp_v6(const struct leases_t *restrict a, const struct leases_t *restrict b);

extern int comp_cur(struct range_t *r1, struct range_t *r2) _DP_ATTRIBUTE_PURE;
extern int comp_double(double f1, double f2) _DP_ATTRIBUTE_CONST;
extern int comp_ip(struct range_t *r1, struct range_t *r2);
extern int comp_max(struct range_t *r1, struct range_t *r2);
extern int comp_percent(struct range_t *r1, struct range_t *r2);
extern int comp_tc(struct range_t *r1, struct range_t *r2) _DP_ATTRIBUTE_PURE;
extern int comp_tcperc(struct range_t *r1, struct range_t *r2);
extern int comp_touched(struct range_t *r1, struct range_t *r2) _DP_ATTRIBUTE_PURE;
extern int rangecomp(const void *__restrict r1, const void *__restrict r2)
    __attribute__ ((nonnull(1, 2)));
/* sort function pointer and functions */
extern comparer_t field_selector(char c);
extern double ret_percent(struct range_t r);
extern double ret_tc(struct range_t r) _DP_ATTRIBUTE_CONST;
extern double ret_tcperc(struct range_t r);
extern void mergesort_ranges(struct range_t *__restrict orig, int size,
			     struct range_t *__restrict temp)
    __attribute__ ((nonnull(1, 3)));
/* output function pointer and functions */
extern int (*output_analysis) (void);
extern int output_txt(void);
extern int output_html(void);
extern int output_xml(void);
extern int output_json(void);
extern int output_csv(void);
extern int output_alarming(void);
/* Memory release, file closing etc */
extern void clean_up(void);
/* Hash functions */
extern void (*add_lease) (union ipaddr_t *ip, enum ltype type);
extern void add_lease_init(union ipaddr_t *ip, enum ltype type) _DP_ATTRIBUTE_CONST;
extern void add_lease_v4(union ipaddr_t *ip, enum ltype type);
extern void add_lease_v6(union ipaddr_t *ip, enum ltype type);

extern struct leases_t *(*find_lease) (union ipaddr_t *ip);
extern struct leases_t *find_lease_init(union ipaddr_t *ip) _DP_ATTRIBUTE_CONST;
extern struct leases_t *find_lease_v4(union ipaddr_t *ip) _DP_ATTRIBUTE_PURE;
extern struct leases_t *find_lease_v6(union ipaddr_t *ip) _DP_ATTRIBUTE_PURE;

extern void delete_lease(struct leases_t *lease);
extern void delete_all_leases(void);

#endif				/* DHCPD_POOLS_H */

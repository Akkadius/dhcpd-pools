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
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of Sami Kerola.
 */

#ifndef DHCPD_POOLS_H
# define DHCPD_POOLS_H 1

#include <config.h>
#include <arpa/inet.h>
#include <stddef.h>

/* Feature test switches */
#define _POSIX_SOURCE 1
#define POSIXLY_CORRECT 1

#ifdef	HAVE_STDLIB_H
#else
extern void exit();
extern char *malloc();
#define EXIT_FAILURE	1
#define EXIT_SUCCESS	0
#endif				/* STDC_HEADERS */

#ifndef HAVE_PROGRAM_INVOCATION_SHORT_NAME
#  ifdef HAVE___PROGNAME
extern char *__progname;
#    define program_invocation_short_name __progname
#  else				/* HAVE___PROGNAME */
#    ifdef HAVE_GETEXECNAME
#      include <stdlib.h>
#      define program_invocation_short_name \
       prog_inv_sh_nm_from_file(getexecname(), 0)
#    else			/* HAVE_GETEXECNAME */
#      define program_invocation_short_name \
       prog_inv_sh_nm_from_file(__FILE__, 1)
#    endif			/* HAVE_PROGRAM_INVOCATION_SHORT_NAME */
static char prog_inv_sh_nm_buf[256];
static inline char *prog_inv_sh_nm_from_file(char *f, char stripext)
{
	char *t;
	if ((t = strrchr(f, '/')) != NULL) {
		t++;
	} else {
		t = f;
	}
	strncpy(prog_inv_sh_nm_buf, t, sizeof(prog_inv_sh_nm_buf) - 1);
	prog_inv_sh_nm_buf[sizeof(prog_inv_sh_nm_buf) - 1] = '\0';

	if (stripext && (t = strrchr(prog_inv_sh_nm_buf, '.')) != NULL) {
		*t = '\0';
	}
	return prog_inv_sh_nm_buf;
}
#  endif
#endif

/* Structures and unions */
struct configuration_t {
	char *dhcpdconf_file;
	char *dhcpdlease_file;
	char output_format[2];
	char sort[6];
	int reverse_order;
	char *output_file;
	int output_limit[2];
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
	uint32_t first_ip;
	uint32_t last_ip;
	unsigned long int count;
	unsigned long int touched;
	unsigned long int backups;
};
struct macaddr_t {
	char *ethernet;
	char *ip;
	struct macaddr_t *next;
};

/* Global variables */
static int const true = 1;
static int const false = 0;

struct configuration_t config;

static int const output_limit_bit_1 = 1;
static int const output_limit_bit_2 = 2;
static int const output_limit_bit_3 = 4;
unsigned int fullhtml;

struct shared_network_t *shared_networks;
unsigned int num_shared_networks;

struct range_t *ranges;
unsigned int num_ranges;

uint32_t *leases;
unsigned long int num_leases;

uint32_t *touches;
unsigned long int num_touches;

uint32_t *backups;
unsigned long int num_backups;

struct macaddr_t *macaddr;

/* Function prototypes */
int prepare_memory(void);
int parse_leases(void);
void parse_config(int, char *, struct shared_network_t *)
    __attribute__ ((nonnull(2, 3)));
int nth_field(int n, char *dest, const char *src)
    __attribute__ ((nonnull(2, 3)))
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
    __attribute__ ((__hot__))
#endif
    ;
int prepare_data(void);
int do_counting(void);
void flip_ranges(struct range_t *ranges, struct range_t *tmp_ranges)
    __attribute__ ((nonnull(1, 2)));
/* General support functions */
void *safe_malloc(const size_t size)
#if __GNUC__ >= 3
    __attribute__ ((__malloc__))
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
    __attribute__ ((__alloc_size__((1))))
#endif
#endif
    ;
void *safe_realloc(void *ptr, const size_t size);
char *safe_strdup(const char *str) __attribute__ ((nonnull(1)));
void print_version(void) __attribute__ ((noreturn));
void usage(int status) __attribute__ ((noreturn));
/* qsort required functions... */
/* ...for ranges and... */
int intcomp(const void *x, const void *y) __attribute__ ((nonnull(1, 2)));
int rangecomp(const void *r1, const void *r2)
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
int get_order(struct range_t *left, struct range_t *right)
    __attribute__ ((nonnull(1, 2)));
void mergesort_ranges(struct range_t *orig, int size, struct range_t *temp)
    __attribute__ ((nonnull(1, 3)));
/* output function pointer and functions */
int (*output_analysis) (void);
int output_txt(void);
int output_html(void);
int output_xml(void);
int output_csv(void);
/* Memory release, file closing etc */
void clean_up(void);

#endif				/* DHCPD_POOLS_H */

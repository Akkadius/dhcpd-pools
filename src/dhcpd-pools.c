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

/*! \file dhcpd-pools.c
 * \brief The main(), and core initialization.
 */

#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdio.h>
#include <limits.h>

#include "close-stream.h"
#include "closeout.h"
#include "error.h"
#include "progname.h"
#include "quote.h"
#include "xalloc.h"

#include "dhcpd-pools.h"
#include "defaults.h"

/* Global variables */
int prefix_length[2][NUM_OF_PREFIX];
struct configuration_t config;
struct shared_network_t *shared_networks;
unsigned int num_shared_networks;
struct range_t *ranges;
unsigned int num_ranges;
struct leases_t *leases;
unsigned int RANGES;

/* Function pointers */
int (*parse_ipaddr) (const char *restrict src, union ipaddr_t *restrict dst);
void (*copy_ipaddr) (union ipaddr_t *restrict dst, const union ipaddr_t *restrict src);
const char *(*ntop_ipaddr) (const union ipaddr_t *ip);
double (*get_range_size) (const struct range_t *r);
int (*xstrstr) (const char *restrict str);
int (*ipcomp) (const union ipaddr_t *restrict a, const union ipaddr_t *restrict b);
int (*leasecomp) (const struct leases_t *restrict a, const struct leases_t *restrict b);
int (*output_analysis) (void);
void (*add_lease) (union ipaddr_t *ip, enum ltype type);
struct leases_t *(*find_lease) (union ipaddr_t *ip);

static int return_limit(const char c)
{
	if ('0' <= c && c < '8')
		return c - '0';
	clean_up();
	error(EXIT_FAILURE, 0, "return_limit: output mask %s is illegal", quote(optarg));
	return 0;
}

/*! \brief Start of execution.  Parse options, and call other other
 * functions one after another.  At the moment adding threading support
 * would be difficult, but there does not seem to be valid reason to
 * consider that.  Overall the analysis already quick enough even without
 * making it parallel.
 *
 * \return Return value indicates success or fail or analysis, unless
 * either --warning or --critical options are in use, which makes the
 * return value in some cases to match with Nagios expectations about
 * alarming. */
int main(int argc, char **argv)
{
	int option_index = 0;
	char const *tmp;
	const char *print_mac_addreses_tmp;
	struct range_t *tmp_ranges;
	enum {
		OPT_SNET_ALARMS = CHAR_MAX + 1,
		OPT_WARN,
		OPT_CRIT,
		OPT_MINSIZE,
		OPT_WARN_COUNT,
		OPT_CRIT_COUNT
	};
	int ret_val;

	static struct option const long_options[] = {
		{"config", required_argument, NULL, 'c'},
		{"leases", required_argument, NULL, 'l'},
		{"format", required_argument, NULL, 'f'},
		{"sort", required_argument, NULL, 's'},
		{"reverse", no_argument, NULL, 'r'},
		{"output", required_argument, NULL, 'o'},
		{"limit", required_argument, NULL, 'L'},
		{"version", no_argument, NULL, 'v'},
		{"help", no_argument, NULL, 'h'},
		{"snet-alarms", no_argument, NULL, OPT_SNET_ALARMS},
		{"warning", required_argument, NULL, OPT_WARN},
		{"critical", required_argument, NULL, OPT_CRIT},
		{"warn-count", required_argument, NULL, OPT_WARN_COUNT},
		{"crit-count", required_argument, NULL, OPT_CRIT_COUNT},
		{"minsize", required_argument, NULL, OPT_MINSIZE},
		{"perfdata", no_argument, NULL, 'p'},
		{NULL, 0, NULL, 0}
	};

	atexit(close_stdout);
	set_program_name(argv[0]);
	/* FIXME: These allocations should be fully dynamic, e.g., grow
	 * if needed.  */
	config.dhcpdconf_file = xmalloc(sizeof(char) * MAXLEN);
	config.dhcpdlease_file = xmalloc(sizeof(char) * MAXLEN);
	config.output_file = xmalloc(sizeof(char) * MAXLEN);
	/* Make sure string has zero length if there is no
	 * command line option */
	config.output_file[0] = '\0';
	/* Alarming defaults. */
	config.snet_alarms = 0;
	config.warning = ALARM_WARN;
	config.critical = ALARM_CRIT;
	config.warn_count = 0x100000000;	/* == 2^32 that is the entire IPv4 space */
	config.crit_count = 0x100000000;	/* basically turns off the count criteria */
	config.perfdata = 0;
	/* File location defaults */
	strncpy(config.dhcpdconf_file, DHCPDCONF_FILE, MAXLEN - 1);
	strncpy(config.dhcpdlease_file, DHCPDLEASE_FILE, MAXLEN - 1);
	tmp = OUTPUT_LIMIT;
	config.header_limit = (*tmp - '0');
	tmp++;
	config.number_limit = (*tmp - '0');
	/* Make sure some output format is selected by default */
	print_mac_addreses_tmp = OUTPUT_FORMAT;
	/* Default sort order is by IPs small to big */
	config.reverse_order = 0;
	config.backups_found = 0;
	prepare_memory();
	/* Parse command line options */
	while (1) {
		int c;

		c = getopt_long(argc, argv, "c:l:f:o:s:rL:pvh", long_options, &option_index);
		if (c == EOF)
			break;
		switch (c) {
		case 'c':
			/* config file */
			strncpy(config.dhcpdconf_file, optarg, MAXLEN - 1);
			break;
		case 'l':
			/* lease file */
			strncpy(config.dhcpdlease_file, optarg, MAXLEN - 1);
			break;
		case 'f':
			/* Output format */
			print_mac_addreses_tmp = optarg;
			break;
		case 's':
		{
			/* Output sorting option */
			struct output_sort *p = config.sorts;
			size_t len;

			while (p && p->next)
				p = p->next;
			for (len = 0; len < strlen(optarg); len++) {
				if (config.sorts == NULL) {
					config.sorts = xcalloc(1, sizeof(struct output_sort));
					p = config.sorts;
				} else {
					p->next = xcalloc(1, sizeof(struct output_sort));
					p = p->next;
				}
				p->func = field_selector(optarg[len]);
			}
		}
			break;
		case 'r':
			/* What ever sort in reverse order */
			config.reverse_order = 1;
			break;
		case 'o':
			/* Output file */
			strncpy(config.output_file, optarg, MAXLEN - 1);
			break;
		case 'L':
			/* Specification what will be printed */
			config.header_limit = return_limit(optarg[0]);
			config.number_limit = return_limit(optarg[1]);
			break;
		case OPT_SNET_ALARMS:
			config.snet_alarms = 1;
			break;
		case OPT_WARN:
			print_mac_addreses_tmp = "a";
			config.warning = strtod_or_err(optarg, "illegal argument");
			break;
		case OPT_CRIT:
			print_mac_addreses_tmp = "a";
			config.critical = strtod_or_err(optarg, "illegal argument");
			break;
		case OPT_WARN_COUNT:
			print_mac_addreses_tmp = "a";
			config.warn_count = strtod_or_err(optarg, "illegal argument");
			break;
		case OPT_CRIT_COUNT:
			print_mac_addreses_tmp = "a";
			config.crit_count = strtod_or_err(optarg, "illegal argument");
			break;
		case OPT_MINSIZE:
			config.minsize = strtod_or_err(optarg, "illegal argument");
			break;
		case 'p':
			/* Print additional performance data in alarming mode */
			config.perfdata = 1;
			break;
		case 'v':
			/* Print version */
			print_version();
		case 'h':
			/* Print help */
			usage(EXIT_SUCCESS);
		default:
			error(EXIT_FAILURE, 0, "Try %s --help for more information.",
			      program_name);
		}
	}
	/* Output function selection */
	switch (print_mac_addreses_tmp[0]) {
	case 't':
		output_analysis = output_txt;
		config.print_mac_addreses = 0;
		break;
	case 'a':
		output_analysis = output_alarming;
		config.print_mac_addreses = 0;
		break;
	case 'h':
		error(EXIT_FAILURE, 0, "html table only output format is deprecated");
		break;
	case 'H':
		output_analysis = output_html;
		config.print_mac_addreses = 0;
		break;
	case 'x':
		output_analysis = output_xml;
		config.print_mac_addreses = 0;
		break;
	case 'X':
		output_analysis = output_xml;
		config.print_mac_addreses = 1;
		break;
	case 'j':
		output_analysis = output_json;
		config.print_mac_addreses = 0;
		break;
	case 'J':
		output_analysis = output_json;
		config.print_mac_addreses = 1;
		break;
	case 'c':
		output_analysis = output_csv;
		config.print_mac_addreses = 0;
		break;
	default:
		clean_up();
		error(EXIT_FAILURE, 0, "unknown output format: %s", quote(print_mac_addreses_tmp));
	}
	/* Do the job */
	set_ipv_functions(IPvUNKNOWN);
	parse_config(1, config.dhcpdconf_file, shared_networks);
	parse_leases();
	prepare_data();
	do_counting();
	tmp_ranges = xmalloc(sizeof(struct range_t) * num_ranges);
	if (config.sorts != NULL)
		mergesort_ranges(ranges, num_ranges, tmp_ranges);
	if (config.reverse_order == 1)
		flip_ranges(ranges, tmp_ranges);
	free(tmp_ranges);
	ret_val = output_analysis();
	clean_up();
	return (ret_val);
}

/*! \brief Run time initialization. Global allocations, counter
 * initializations, etc are here. */
void prepare_memory(void)
{
	config.ip_version = IPvUNKNOWN;
	RANGES = 64;
	num_ranges = num_shared_networks = 0;
	shared_networks = xmalloc(sizeof(struct shared_network_t) * SHARED_NETWORKS);
	ranges = xmalloc(sizeof(struct range_t) * RANGES);
	/* First shared network entry is all networks */
	shared_networks->name = xstrdup("All networks");
	shared_networks->used = 0;
	shared_networks->touched = 0;
	shared_networks->backups = 0;
	config.sorts = NULL;
}

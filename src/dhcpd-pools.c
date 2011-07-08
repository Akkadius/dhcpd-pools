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

#include <config.h>

#ifdef  HAVE_STDLIB_H
#include <stdlib.h>
#else
extern char *malloc();
#endif

#ifdef  HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>

#include "defaults.h"
#include "dhcpd-pools.h"

int main(int argc, char **argv)
{
	int i, c, sorts = 0;
	int option_index = 0;
	char *tmp;
	struct range_t *tmp_ranges;

	/* Options for getopt_long */
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
		{NULL, 0, NULL, 0}
	};

	/* FIXME: make these allocations dynamic up on need. */
	config.dhcpdconf_file = safe_malloc(sizeof(char) * MAXLEN);
	config.dhcpdlease_file = safe_malloc(sizeof(char) * MAXLEN);
	config.output_file = safe_malloc(sizeof(char) * MAXLEN);

	/* Make sure string has zero lenght if there is no
	 * command line option */
	config.output_file[0] = '\0';

	/* File location defaults */
	strncpy(config.dhcpdconf_file, DHCPDCONF_FILE, MAXLEN - 1);
	strncpy(config.dhcpdlease_file, DHCPDLEASE_FILE, MAXLEN - 1);
	tmp = OUTPUT_LIMIT;
	config.output_limit[0] = (*tmp - '0');
	tmp++;
	config.output_limit[1] = (*tmp - '0');
	fullhtml = false;

	/* Make sure some output format is selected by default */
	strncpy(config.output_format, OUTPUT_FORMAT, (size_t) 1);

	/* Default sort order is by IPs small to big */
	config.reverse_order = false;

	/* Parse command line options */
	while (1) {
		c = getopt_long(argc, argv, "c:l:f:o:s:rL:vh",
				long_options, &option_index);

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
			strncpy(config.output_format, optarg, (size_t) 1);
			break;
		case 's':
			/* Output sorting option */
			sorts = strlen(optarg);
			if (5 < sorts) {
				warnx
				    ("main: only first 5 sort orders will be used");
				strncpy(config.sort, optarg, (size_t) 5);
				sorts = 5;
			} else {
				strncpy(config.sort, optarg, (size_t) sorts);
			}
			for (i = 0; i < sorts; i++) {
				field_selector(config.sort[i]);
			}
			break;
		case 'r':
			/* What ever sort in reverse order */
			config.reverse_order = true;
			break;
		case 'o':
			/* Output file */
			strncpy(config.output_file, optarg, MAXLEN - 1);
			break;
		case 'L':
			/* Specification what will be printed */
			for (i = 0; i < 2; i++) {
				if (optarg[i] >= '0' && optarg[i] < '8') {
					config.output_limit[i] =
					    optarg[i] - '0';
				} else {
					errx(EXIT_FAILURE,
					     "main: output mask `%s' is illegal",
					     optarg);
				}
			}
			break;
		case 'v':
			/* Print version */
			print_version();
		case 'h':
			/* Print help */
			usage(EXIT_SUCCESS);
		default:
			errx(EXIT_FAILURE,
			     "Try `%s --help' for more information.",
			     program_invocation_short_name);
		}
	}

	/* Output function selection */
	switch (config.output_format[0]) {
	case 't':
		output_analysis = output_txt;
		break;
	case 'h':
		output_analysis = output_html;
		break;
	case 'H':
		output_analysis = output_html;
		fullhtml = true;
		break;
	case 'x':
		output_analysis = output_xml;
		break;
	case 'X':
		output_analysis = output_xml;
		break;
	case 'c':
		output_analysis = output_csv;
		break;
	default:
		errx(EXIT_FAILURE, "main: unknown output format `%c'",
		     config.output_format[0]);
	}

	/* Do the job */
	prepare_memory();
	parse_config(true, config.dhcpdconf_file, shared_networks);

	parse_leases();
	prepare_data();
	do_counting();
	tmp_ranges = safe_malloc(sizeof(struct range_t) * num_ranges);
	if (sorts != 0) {
		mergesort_ranges(ranges, num_ranges, tmp_ranges);
	}
	if (config.reverse_order == true) {
		flip_ranges(ranges, tmp_ranges);
	}
	free(tmp_ranges);
	output_analysis();

	clean_up();
	return (EXIT_SUCCESS);
}

/* Global allocations, counter resets etc */
int prepare_memory()
{
	RANGES = 64;
	num_ranges = num_shared_networks = 0;
	shared_networks =
	    safe_malloc(sizeof(struct shared_network_t) * SHARED_NETWORKS);

	ranges = safe_malloc(sizeof(struct range_t) * RANGES);
	macaddr = NULL;

	/* First shared network entry is all networks */
	shared_networks->name = safe_strdup("All networks");
	shared_networks->used = 0;
	shared_networks->touched = 0;
	shared_networks->backups = 0;

	return 0;
}

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
#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <limits.h>

#include "close-stream.h"
#include "closeout.h"
#include "defaults.h"
#include "dhcpd-pools.h"
#include "progname.h"
#include "xalloc.h"

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
	int i, sorts = 0;
	int option_index = 0;
	char const *tmp;
	struct range_t *tmp_ranges;
	enum {
		OPT_WARN = CHAR_MAX + 1,
		OPT_CRIT
	};
	int ret_val;

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
		{"warning", required_argument, NULL, OPT_WARN},
		{"critical", required_argument, NULL, OPT_CRIT},
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
	config.warning = ALARM_WARN;
	config.critical = ALARM_CRIT;

	/* File location defaults */
	strncpy(config.dhcpdconf_file, DHCPDCONF_FILE, MAXLEN - 1);
	strncpy(config.dhcpdlease_file, DHCPDLEASE_FILE, MAXLEN - 1);
	tmp = OUTPUT_LIMIT;
	config.output_limit[0] = (*tmp - '0');
	tmp++;
	config.output_limit[1] = (*tmp - '0');
	config.fullhtml = false;

	/* Make sure some output format is selected by default */
	strncpy(config.output_format, OUTPUT_FORMAT, (size_t)1);

	/* Default sort order is by IPs small to big */
	config.reverse_order = false;
	config.backups_found = false;

	/* Parse command line options */
	while (1) {
		int c;
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
			strncpy(config.output_format, optarg, (size_t)1);
			break;
		case 's':
			/* Output sorting option */
			sorts = strlen(optarg);
			if (5 < sorts) {
				warnx
				    ("main: only first 5 sort orders will be used");
				strncpy(config.sort, optarg, (size_t)5);
				sorts = 5;
			} else {
				strncpy(config.sort, optarg, (size_t)sorts);
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
		case OPT_WARN:
			strcpy(config.output_format, "a");
			config.warning =
			    strtod_or_err(optarg, "illegal argument");
			break;
		case OPT_CRIT:
			strcpy(config.output_format, "a");
			config.critical =
			    strtod_or_err(optarg, "illegal argument");
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
			     program_name);
		}
	}

	/* Output function selection */
	switch (config.output_format[0]) {
	case 't':
		output_analysis = output_txt;
		break;
	case 'a':
		output_analysis = output_alarming;
		break;
	case 'h':
		output_analysis = output_html;
		break;
	case 'H':
		output_analysis = output_html;
		config.fullhtml = true;
		break;
	case 'x':
		output_analysis = output_xml;
		break;
	case 'X':
		output_analysis = output_xml;
		break;
	case 'j':
		output_analysis = output_json;
		break;
	case 'J':
		output_analysis = output_json;
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
	tmp_ranges = xmalloc(sizeof(struct range_t) * num_ranges);
	if (sorts != 0) {
		mergesort_ranges(ranges, num_ranges, tmp_ranges);
	}
	if (config.reverse_order == true) {
		flip_ranges(ranges, tmp_ranges);
	}
	free(tmp_ranges);
	ret_val = output_analysis();

	clean_up();
	return (ret_val);
}

/*! \brief Run time initialization. Global allocations, counter
 * initializations, etc are here.
 * FIXME: This function should return void. */
int prepare_memory(void)
{
	/* Fill in prefix length cache */
	int i, j;
	for (i = 0; i < 2; i++) {
		for (j = 0; j < NUM_OF_PREFIX; j++) {
			prefix_length[i][j] = strlen(prefixes[i][j]);
		}
	}
	config.dhcp_version = VERSION_UNKNOWN;
	RANGES = 64;
	num_ranges = num_shared_networks = 0;
	shared_networks =
	    xmalloc(sizeof(struct shared_network_t) * SHARED_NETWORKS);

	ranges = xmalloc(sizeof(struct range_t) * RANGES);

	/* First shared network entry is all networks */
	shared_networks->name = xstrdup("All networks");
	shared_networks->used = 0;
	shared_networks->touched = 0;
	shared_networks->backups = 0;

	return 0;
}

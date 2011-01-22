/*
** Copyright (C) 2006- Sami Kerola <kerolasa@iki.fi>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef  HAVE_STDLIB_H
#include <stdlib.h>
#else				/* Not STDC_HEADERS */
extern char *malloc();
#endif				/* STDC_HEADERS */
#ifdef  HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <getopt.h>
#include <errno.h>
#include <err.h>

#include "dhcpd-pools.h"
#include "defaults.h"

int main(int argc, char **argv)
{
	int i, c, sorts = 0;
	int option_index = 0;
	char *tmp;
	struct range_t *tmp_ranges;

	/* Options for getopt_long */
	static struct option const long_options[] = {
		{"config", required_argument, 0, (int) 'c'},
		{"leases", required_argument, 0, (int) 'l'},
		{"format", required_argument, 0, (int) 'f'},
		{"sort", required_argument, 0, (int) 's'},
		{"reverse", no_argument, 0, (int) 'r'},
		{"output", required_argument, 0, (int) 'o'},
		{"limit", required_argument, 0, (int) 'L'},
		{"version", no_argument, 0, (int) 'v'},
		{"help", no_argument, 0, (int) 'h'},
		{0, 0, 0, 0}
	};

	/* FIXME: make these allocations dynamic up on need. */
	config.dhcpdconf_file = safe_malloc(sizeof(char) * MAXLEN);
	config.dhcpdlease_file = safe_malloc(sizeof(char) * MAXLEN);
	config.output_file = safe_malloc(sizeof(char) * MAXLEN);

	/* Make sure string has zero lenght if there is no
	 * command line option */
	config.output_file[0] = '\0';

	/* File location defaults */
	strncpy(config.dhcpdconf_file, DHCPDCONF_FILE,
		(size_t) MAXLEN - 1);
	strncpy(config.dhcpdlease_file, DHCPDLEASE_FILE,
		(size_t) MAXLEN - 1);
	tmp = OUTPUT_LIMIT;
	config.output_limit[0] = (int) (*tmp - '0');
	tmp++;
	config.output_limit[1] = (int) (*tmp - '0');
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
			strncpy(config.dhcpdconf_file, optarg,
				(size_t) MAXLEN - 1);
			break;
		case 'l':
			/* lease file */
			strncpy(config.dhcpdlease_file, optarg,
				(size_t) MAXLEN - 1);
			break;
		case 'f':
			/* Output format */
			strncpy(config.output_format, optarg, (size_t) 1);
			break;
		case 's':
			/* Output sorting option */
			sorts = strlen(optarg);
			if (5 < sorts) {
				warnx("main: only first 5 sort orders will be used");
				strncpy(config.sort, optarg, (size_t) 5);
				sorts = 5;
			} else {
				strncpy(config.sort, optarg,
					(size_t) sorts);
			}
			break;
		case 'r':
			/* What ever sort in reverse order */
			config.reverse_order = true;
			break;
		case 'o':
			/* Output file */
			strncpy(config.output_file, optarg,
				(size_t) MAXLEN - 1);
			break;
		case 'L':
			/* Specification what will be printed */
			for (i = 0; i < 2; i++) {
				if (optarg[i] >= '0' && optarg[i] < '8') {
					config.output_limit[i] =
					    (int) optarg[i] - '0';
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
			return (EXIT_SUCCESS);
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
		errx(EXIT_FAILURE, "main: unknown ouput format `%c'",
		     config.output_format[0]);
	}

	/* Do the job */
	prepare_memory();
	parse_config(true, config.dhcpdconf_file, shared_net_names,
		     shared_net_names + strlen(shared_net_names) + 1,
		     shared_networks);

	/* FIXME: move to output.c and use FILE *outfile */
	if ((config.output_format[0] == 'x')
	    || (config.output_format[0] == 'X')) {
		printf("<dhcpstatus>\n");
	};

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
	/* After fopen in ouput ioctl does like /dev/null which
	 * cause ENOTTY, and clean_up will see that without this
	 * reset. At least linux does this, and possibly some
	 * other systems. There's a report from FreeBSD 8.0 which
	 * matches quite well with the symptom. */
	if (errno == 25)
		errno = 0;

	/* FIXME: move to output.c and use FILE *outfile */
	if ((config.output_format[0] == 'x')
	    || (config.output_format[0] == 'X')) {
		printf("</dhcpstatus>\n");
	};

	clean_up();
	return (EXIT_SUCCESS);
}

/* Global allocations, counter resets etc */
int prepare_memory()
{
	num_ranges = num_shared_networks = 0;
	shared_networks =
	    safe_malloc(sizeof(struct shared_network_t) * SHARED_NETWORKS);
	shared_net_names =
	    safe_malloc(sizeof(char) * SHARED_NETWORKS_NAMES);

	ranges = safe_malloc(sizeof(struct range_t) * RANGES);

	/* First shared network entry is all networks */
	strcpy(shared_net_names, "All networks");
	return 0;
}

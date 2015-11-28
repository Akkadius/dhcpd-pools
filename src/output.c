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

/*! \file output.c
 * \brief All about output formats.
 */

#include <config.h>

#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <langinfo.h>
#include <math.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

#include "close-stream.h"
#include "error.h"
#include "progname.h"
#include "strftime.h"

#include "dhcpd-pools.h"

/*! \brief Text output format, which is the default.
 * FIXME: This function should return void. */
int output_txt(void)
{
	unsigned int i;
	struct range_t *range_p;
	double range_size;
	struct shared_network_t *shared_p;
	int ret;
	FILE *outfile;
	int max_ipaddr_length = config.dhcp_version == VERSION_6 ? 39 : 16;

	if (config.output_file[0]) {
		outfile = fopen(config.output_file, "w+");
		if (outfile == NULL) {
			error(EXIT_FAILURE, errno, "output_txt: %s", config.output_file);
		}
	} else {
		outfile = stdout;
	}

	range_p = ranges;
	range_size = get_range_size(range_p);
	shared_p = shared_networks;

	if (config.output_limit[0] & BIT1) {
		fprintf(outfile, "Ranges:\n");
		fprintf
		    (outfile,
		     "%-20s%-*s   %-*s %5s %5s %10s  %5s %5s %9s",
		     "shared net name",
		     max_ipaddr_length,
		     "first ip",
		     max_ipaddr_length,
		     "last ip", "max", "cur", "percent", "touch", "t+c", "t+c perc");
		if (config.backups_found == true) {
			fprintf(outfile, "     bu  bu perc");
		}
		fprintf(outfile, "\n");
	}
	if (config.output_limit[1] & BIT1) {
		for (i = 0; i < num_ranges; i++) {
			if (range_p->shared_net) {
				fprintf(outfile, "%-20s", range_p->shared_net->name);
			} else {
				fprintf(outfile, "not_defined         ");
			}
			/* Outputting of first_ip and last_ip need to be
			 * separate since ntop_ipaddr always returns the
			 * same buffer */
			fprintf(outfile, "%-*s",
				max_ipaddr_length, ntop_ipaddr(&range_p->first_ip));
			fprintf(outfile,
				" - %-*s %5g %5g %10.3f  %5g %5g %9.3f",
				max_ipaddr_length,
				ntop_ipaddr(&range_p->last_ip),
				range_size,
				range_p->count,
				(float)(100 * range_p->count) / range_size,
				range_p->touched,
				range_p->touched + range_p->count,
				(float)(100 * (range_p->touched + range_p->count)) / range_size);
			if (config.backups_found == true) {
				fprintf(outfile, "%7g %8.3f",
					range_p->backups,
					(float)(100 * range_p->backups) / range_size);
			}
			fprintf(outfile, "\n");
			range_p++;
			range_size = get_range_size(range_p);
		}
	}
	if (config.output_limit[1] & BIT1 && config.output_limit[0] & BIT2) {
		fprintf(outfile, "\n");
	}
	if (config.output_limit[0] & BIT2) {
		fprintf(outfile, "Shared networks:\n");
		fprintf(outfile,
			"name                   max   cur     percent  touch    t+c  t+c perc");
		if (config.backups_found == true) {
			fprintf(outfile, "     bu  bu perc");
		}
		fprintf(outfile, "\n");
	}
	if (config.output_limit[1] & BIT2) {
		for (i = 0; i < num_shared_networks; i++) {
			shared_p++;
			fprintf(outfile,
				"%-20s %5g %5g %10.3f %7g %6g %9.3f",
				shared_p->name, shared_p->available,
				shared_p->used,
				shared_p->available ==
				0 ? -NAN : (float)(100 * shared_p->used) / shared_p->available,
				shared_p->touched, shared_p->touched + shared_p->used,
				shared_p->available ==
				0 ? -NAN : ((float)(100 * (shared_p->touched + shared_p->used)) /
					    shared_p->available));
			if (config.backups_found == true) {
				fprintf(outfile, "%7g %8.3f",
					shared_p->backups,
					(float)(100 * shared_p->backups) / shared_p->available);
			}

			fprintf(outfile, "\n");
		}
	}
	if (config.output_limit[1] & BIT2 && config.output_limit[0] & BIT3) {
		fprintf(outfile, "\n");
	}
	if (config.output_limit[0] & BIT3) {
		fprintf(outfile, "Sum of all ranges:\n");
		fprintf(outfile,
			"name                   max   cur     percent  touch    t+c  t+c perc");

		if (config.backups_found == true) {
			fprintf(outfile, "     bu  bu perc");
		}
		fprintf(outfile, "\n");
	}
	if (config.output_limit[1] & BIT3) {
		fprintf(outfile, "%-20s %5g %5g %10.3f %7g %6g %9.3f",
			shared_networks->name,
			shared_networks->available,
			shared_networks->used,
			shared_networks->available ==
			0 ? -NAN : (float)(100 * shared_networks->used) /
			shared_networks->available, shared_networks->touched,
			shared_networks->touched + shared_networks->used,
			shared_networks->available ==
			0 ? -NAN : (float)(100 *
					   (shared_networks->touched +
					    shared_networks->used)) / shared_networks->available);

		if (config.backups_found == true) {
			fprintf(outfile, "%7g %8.3f",
				shared_networks->available == 0 ? -NAN : shared_networks->backups,
				(float)(100 * shared_networks->backups) /
				shared_networks->available);
		}
		fprintf(outfile, "\n");
	}
	if (outfile == stdout) {
		ret = fflush(stdout);
		if (ret) {
			error(0, 0, "output_txt: fflush");
		}
	} else {
		ret = close_stream(outfile);
		if (ret) {
			error(0, 0, "output_txt: fclose");
		}
	}

	return 0;
}

/*! \brief The xml output formats.
 * FIXME: This function should return void.
 */
int output_xml(void)
{
	unsigned int i;
	struct range_t *range_p;
	double range_size;
	struct shared_network_t *shared_p;
	int ret;
	FILE *outfile;

	if (config.output_file[0]) {
		outfile = fopen(config.output_file, "w+");
		if (outfile == NULL) {
			error(EXIT_FAILURE, errno, "output_xml: %s", config.output_file);
		}
	} else {
		outfile = stdout;
	}

	range_p = ranges;
	range_size = get_range_size(range_p);
	shared_p = shared_networks;

	fprintf(outfile, "<dhcpstatus>\n");

	if (config.output_format[0] == 'X' || config.output_format[0] == 'J') {
		struct leases_t *l;

		for (l = leases; l != NULL; l = l->hh.next) {
			if (l->type == ACTIVE) {
				fputs("<active_lease>\n\t<ip>", outfile);
				fputs(ntop_ipaddr(&l->ip), outfile);
				fputs("</ip>\n\t<macaddress>", outfile);
				if (l->ethernet != NULL) {
					fputs(l->ethernet, outfile);
				}
				fputs("</macaddress>\n</active_lease>\n", outfile);
			}
		}
	}

	if (config.output_limit[1] & BIT1) {
		for (i = 0; i < num_ranges; i++) {
			fprintf(outfile, "<subnet>\n");
			if (range_p->shared_net) {
				fprintf(outfile,
					"\t<location>%s</location>\n", range_p->shared_net->name);
			} else {
				fprintf(outfile, "\t<location></location>\n");
			}
			fprintf(outfile, "\t<range>%s ", ntop_ipaddr(&range_p->first_ip));
			fprintf(outfile, "- %s</range>\n", ntop_ipaddr(&range_p->last_ip));
			fprintf(outfile, "\t<defined>%g</defined>\n", range_size);
			fprintf(outfile, "\t<used>%g</used>\n", range_p->count);
			fprintf(outfile, "\t<touched>%g</touched>\n", range_p->touched);
			fprintf(outfile, "\t<free>%g</free>\n", range_size - range_p->count);
			range_p++;
			range_size = get_range_size(range_p);
			fprintf(outfile, "</subnet>\n");
		}
	}

	if (config.output_limit[1] & BIT2) {
		for (i = 0; i < num_shared_networks; i++) {
			shared_p++;
			fprintf(outfile, "<shared-network>\n");
			fprintf(outfile, "\t<location>%s</location>\n", shared_p->name);
			fprintf(outfile, "\t<defined>%g</defined>\n", shared_p->available);
			fprintf(outfile, "\t<used>%g</used>\n", shared_p->used);
			fprintf(outfile, "\t<touched>%g</touched>\n", shared_p->touched);
			fprintf(outfile, "\t<free>%g</free>\n",
				shared_p->available - shared_p->used);
			fprintf(outfile, "</shared-network>\n");
		}
	}

	if (config.output_limit[0] & BIT3) {
		fprintf(outfile, "<summary>\n");
		fprintf(outfile, "\t<location>%s</location>\n", shared_networks->name);
		fprintf(outfile, "\t<defined>%g</defined>\n", shared_networks->available);
		fprintf(outfile, "\t<used>%g</used>\n", shared_networks->used);
		fprintf(outfile, "\t<touched>%g</touched>\n", shared_networks->touched);
		fprintf(outfile, "\t<free>%g</free>\n",
			shared_networks->available - shared_networks->used);
		fprintf(outfile, "</summary>\n");
	}

	fprintf(outfile, "</dhcpstatus>\n");
	if (outfile == stdout) {
		ret = fflush(stdout);
		if (ret) {
			error(0, 0, "output_xml: fflush");
		}
	} else {
		ret = close_stream(outfile);
		if (ret) {
			error(0, 0, "output_xml: fclose");
		}
	}

	return 0;
}

/*! \brief The json output formats.
 * FIXME: This function should return void.
 */
int output_json(void)
{
	unsigned int i = 0;
	struct range_t *range_p;
	double range_size;
	struct shared_network_t *shared_p;
	int ret;
	FILE *outfile;
	unsigned int sep;

	if (config.output_file[0]) {
		outfile = fopen(config.output_file, "w+");
		if (outfile == NULL) {
			error(EXIT_FAILURE, errno, "output_json: %s", config.output_file);
		}
	} else {
		outfile = stdout;
	}

	range_p = ranges;
	range_size = get_range_size(range_p);
	shared_p = shared_networks;
	sep = 0;

	fprintf(outfile, "{\n");

	if (config.output_format[0] == 'X' || config.output_format[0] == 'J') {
		struct leases_t *l;

		fprintf(outfile, "   \"active_leases\": [");
		for (l = leases; l != NULL; l = l->hh.next) {
			if (l->type == ACTIVE) {
				if (i == 0) {
					i = 1;
				} else {
					fputc(',', outfile);
				}
				fputs("\n         { \"ip\":\"", outfile);
				fputs(ntop_ipaddr(&l->ip), outfile);
				fputs("\", \"macaddress\":\"", outfile);
				if (l->ethernet != NULL) {
					fputs(l->ethernet, outfile);
				}
				fputs("\" }", outfile);
			}
		}
		fprintf(outfile, "\n   ]");	/* end of active_leases */
		sep++;
	}

	if (config.output_limit[1] & BIT1) {
		if (sep) {
			fprintf(outfile, ",\n");
		}
		fprintf(outfile, "   \"subnets\": [\n");
		for (i = 0; i < num_ranges; i++) {
			fprintf(outfile, "         ");
			fprintf(outfile, "{ ");
			if (range_p->shared_net) {
				fprintf(outfile,
					"\"location\":\"%s\", ", range_p->shared_net->name);
			} else {
				fprintf(outfile, "\"location\":\"\", ");
			}

			fprintf(outfile, "\"range\":\"%s", ntop_ipaddr(&range_p->first_ip));
			fprintf(outfile, " - %s\", ", ntop_ipaddr(&range_p->last_ip));
			fprintf(outfile, "\"defined\":%g, ", range_size);
			fprintf(outfile, "\"used\":%g, ", range_p->count);
			fprintf(outfile, "\"touched\":%g, ", range_p->touched);
			fprintf(outfile, "\"free\":%g ", range_size - range_p->count);
			range_p++;
			range_size = get_range_size(range_p);
			if (i + 1 < num_ranges)
				fprintf(outfile, "},\n");
			else
				fprintf(outfile, "}\n");
		}
		fprintf(outfile, "   ]");	/* end of subnets */
		sep++;
	}

	if (config.output_limit[1] & BIT2) {
		if (sep) {
			fprintf(outfile, ",\n");
		}
		fprintf(outfile, "   \"shared-networks\": [\n");
		for (i = 0; i < num_shared_networks; i++) {
			fprintf(outfile, "         ");
			shared_p++;
			fprintf(outfile, "{ ");
			fprintf(outfile, "\"location\":\"%s\", ", shared_p->name);
			fprintf(outfile, "\"defined\":%g, ", shared_p->available);
			fprintf(outfile, "\"used\":%g, ", shared_p->used);
			fprintf(outfile, "\"touched\":%g, ", shared_p->touched);
			fprintf(outfile, "\"free\":%g ", shared_p->available - shared_p->used);
			if (i + 1 < num_shared_networks)
				fprintf(outfile, "},\n");
			else
				fprintf(outfile, "}\n");
		}
		fprintf(outfile, "   ]");	/* end of shared-networks */
		sep++;
	}

	if (config.output_limit[0] & BIT3) {
		if (sep) {
			fprintf(outfile, ",\n");
		}
		fprintf(outfile, "   \"summary\": {\n");
		fprintf(outfile, "         \"location\":\"%s\",\n", shared_networks->name);
		fprintf(outfile, "         \"defined\":%g,\n", shared_networks->available);
		fprintf(outfile, "         \"used\":%g,\n", shared_networks->used);
		fprintf(outfile, "         \"touched\":%g,\n", shared_networks->touched);
		fprintf(outfile, "         \"free\":%g\n",
			shared_networks->available - shared_networks->used);
		fprintf(outfile, "   }");	/* end of summary */
		sep++;
	}

	fprintf(outfile, "\n}\n");
	if (outfile == stdout) {
		ret = fflush(stdout);
		if (ret) {
			error(0, 0, "output_json: fflush");
		}
	} else {
		ret = close_stream(outfile);
		if (ret) {
			error(0, 0, "output_json: fclose");
		}
	}

	return 0;
}

/*! \brief Header for full html output format.
 *
 * \param f Output file descriptor.
 */
static void html_header(FILE *restrict f)
{
	char outstr[200];
	struct tm *tmp, result;

	struct stat statbuf;

	stat(config.dhcpdlease_file, &statbuf);

	tmp = localtime_r(&statbuf.st_mtime, &result);
	if (tmp == NULL) {
		error(EXIT_FAILURE, errno, "html_header: localtime");
	}
	if (strftime(outstr, sizeof(outstr), nl_langinfo(D_T_FMT), &result) == 0) {
		error(EXIT_FAILURE, 0, "html_header: strftime returned 0");
	}
	fprintf(f, "<!DOCTYPE html>\n");
	fprintf(f, "<html>\n");
	fprintf(f, "<head>\n");
	fprintf(f, "<title>ISC dhcpd dhcpd-pools output</title>\n");
	fprintf(f, "<meta charset=\"utf-8\">\n");
	fprintf(f, "<meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">\n");
	fprintf(f, "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n");
	fprintf(f, "<link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.4/css/bootstrap.min.css\" type=\"text/css\">\n");
	fprintf(f, "<link rel=\"stylesheet\" href=\"https://cdn.datatables.net/1.10.10/css/jquery.dataTables.min.css\" type=\"text/css\">");
	fprintf(f, "<style type=\"text/css\">\n");
	fprintf(f, "table.dhcpd-pools th { text-transform: capitalize }\n");
	fprintf(f, "</style>\n");
	fprintf(f, "</head>\n");
	fprintf(f, "<body>\n");
	fprintf(f, "<div class=\"container\">\n");
	fprintf(f, "<h2>ISC DHCPD status</h2>\n");
	fprintf(f, "<small>File %s was last modified at %s</small><hr />\n", config.dhcpdlease_file, outstr);
}

/*! \brief Footer for full html output format.
 *
 * \param f Output file descriptor.
 */
static void html_footer(FILE *restrict f)
{
	fprintf(f, "<br /><div class=\"well well-lg\">\n");
	fprintf(f, "<small>Generated using %s<br />\n", PACKAGE_STRING);
	fprintf(f, "More info at <a href=\"%s\">%s</a>\n", PACKAGE_URL, PACKAGE_URL);
	fprintf(f, "</small></div></div>\n");
	fprintf(f, "<script src=\"//code.jquery.com/jquery-2.1.4.min.js\" type=\"text/javascript\"></script>\n");
	fprintf(f, "<script src=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.4/js/bootstrap.min.js\" type=\"text/javascript\"></script>\n");
	fprintf(f, "<script src=\"https://cdn.datatables.net/1.10.10/js/jquery.dataTables.min.js\" type=\"text/javascript\"></script>\n");
	fprintf(f, "<script type=\"text/javascript\" class=\"init\">$(document).ready(function() { $('#s').DataTable(); } );</script>\n");
	fprintf(f, "<script type=\"text/javascript\" class=\"init\">$(document).ready(function() { $('#r').DataTable(); } );</script>\n");
	fprintf(f, "</body></html>\n");
}

/*! \brief Start a html tag.
 *
 * \param f Output file descriptor.
 * \param tag The html tag.
 */
static void start_tag(FILE *restrict f, char const *restrict tag)
{
	fprintf(f, "<%s>\n", tag);
}

/*! \brief End a html tag.
 *
 * \param f Output file descriptor.
 * \param tag The html tag.
 */
static void end_tag(FILE *restrict f, char const *restrict tag)
{
	fprintf(f, "</%s>\n", tag);
}

/*! \brief Line with text in html output format.
 *
 * \param f Output file descriptor.
 * \param type HTML tag name.
 * \param class How the data is aligned.
 * \param text Actual payload of the printout.
 */
static void output_line(FILE *restrict f, char const *restrict type,
			char const *restrict text)
{
	fprintf(f, "<%s>%s</%s>\n", type, text, type);
}

/*! \brief Line with digit in html output format.
 *
 * \param f Output file descriptor.
 * \param type HMTL tag name.
 * \param d Actual payload of the printout.
 */
static void output_double(FILE *restrict f, char const *restrict type, double d)
{
	fprintf(f, "<%s>%g</%s>\n", type, d, type);
}

/*! \brief Line with float in html output format.
 *
 * \param f Output file descriptor.
 * \param type HTML tag name.
 * \param fl Actual payload of the printout.
 */
static void output_float(FILE *restrict f, char const *restrict type, float fl)
{
	fprintf(f, "<%s>%.3f</%s>\n", type, fl, type);
}

/*! \brief Begin table in html output format.
 *
 * \param f Output file descriptor.
 */
static void table_start(FILE *restrict f, char const *restrict id, char const *restrict summary)
{
	fprintf(f, "<table id=\"%s\" class=\"dhcpd-pools order-column table table-striped table-hover\" summary=\"%s\">\n", id, summary);
}

/*! \brief End table in html output format.
 *
 * \param f Output file descriptor.
 */
static void table_end(FILE *restrict f)
{
	fprintf(f, "</table>\n");
}

/*! \brief New section in html output format.
 *
 * \param f Output file descriptor.
 * \param title Table title.
 */
static void newsection(FILE *restrict f, char const *restrict title)
{
	output_line(f, "h3", title);
}

/*! \brief Output html format.
 * FIXME: This function should return void.
 */
int output_html(void)
{
	unsigned int i;
	struct range_t *range_p;
	double range_size;
	struct shared_network_t *shared_p;
	int ret;
	FILE *outfile;

	if (config.output_file[0]) {
		outfile = fopen(config.output_file, "w+");
		if (outfile == NULL) {
			error(EXIT_FAILURE, errno, "output_html: %s", config.output_file);
		}
	} else {
		outfile = stdout;
	}

	range_p = ranges;
	range_size = get_range_size(range_p);
	shared_p = shared_networks;
	html_header(outfile);
	newsection(outfile, "Sum of all");
	table_start(outfile, "a", "all");
	if (config.output_limit[0] & BIT3) {
		start_tag(outfile, "thead");
		start_tag(outfile, "tr");
		output_line(outfile, "th", "name");
		output_line(outfile, "th", "max");
		output_line(outfile, "th", "cur");
		output_line(outfile, "th", "percent");
		output_line(outfile, "th", "touch");
		output_line(outfile, "th", "t+c");
		output_line(outfile, "th", "t+c perc");
		if (config.backups_found == true) {
			output_line(outfile, "th", "bu");
			output_line(outfile, "th", "bu perc");
		}
		end_tag(outfile, "tr");
		end_tag(outfile, "thead");
	}
	if (config.output_limit[1] & BIT3) {
		start_tag(outfile, "tbody");
		start_tag(outfile, "tr");
		output_line(outfile, "td", shared_networks->name);
		output_double(outfile, "td", shared_networks->available);
		output_double(outfile, "td", shared_networks->used);
		output_float(outfile, "td",
			     shared_networks->available ==
			     0 ? -NAN : (float)(100 * shared_networks->used) /
			     shared_networks->available);
		output_double(outfile, "td", shared_networks->touched);
		output_double(outfile, "td", shared_networks->touched + shared_networks->used);
		output_float(outfile, "td",
			     shared_networks->available == 0 ? -NAN : (float)(100 *
									      (shared_networks->touched
									       +
									       shared_networks->used))
			     / shared_networks->available);
		if (config.backups_found == true) {
			output_double(outfile, "td", shared_networks->backups);
			output_float(outfile, "td",
				     shared_networks->available == 0 ? -NAN : (float)(100 *
										      shared_networks->backups)
				     / shared_networks->available);
		}
		end_tag(outfile, "tr");
		end_tag(outfile, "tbody");
	}
	table_end(outfile);
	newsection(outfile, "Shared networks");
	table_start(outfile, "s", "snet");
	if (config.output_limit[0] & BIT2) {
		start_tag(outfile, "thead");
		start_tag(outfile, "tr");
		output_line(outfile, "th", "name");
		output_line(outfile, "th", "max");
		output_line(outfile, "th", "cur");
		output_line(outfile, "th", "percent");
		output_line(outfile, "th", "touch");
		output_line(outfile, "th", "t+c");
		output_line(outfile, "th", "t+c perc");
		if (config.backups_found == true) {
			output_line(outfile, "th", "bu");
			output_line(outfile, "th", "bu perc");
		}
		end_tag(outfile, "tr");
		end_tag(outfile, "thead");
	}
	if (config.output_limit[1] & BIT2) {
		start_tag(outfile, "tbody");
		for (i = 0; i < num_shared_networks; i++) {
			shared_p++;
			start_tag(outfile, "tr");
			output_line(outfile, "td", shared_p->name);
			output_double(outfile, "td", shared_p->available);
			output_double(outfile, "td", shared_p->used);
			output_float(outfile, "td",
				     shared_p->available ==
				     0 ? -NAN : (float)(100 * shared_p->used) /
				     shared_p->available);
			output_double(outfile, "td", shared_p->touched);
			output_double(outfile, "td", shared_p->touched + shared_p->used);
			output_float(outfile, "td",
				     shared_p->available == 0 ? -NAN : (float)(100 *
									       (shared_p->touched +
										shared_p->used)) /
				     shared_p->available);
			if (config.backups_found == true) {
				output_double(outfile, "td", shared_p->backups);
				output_float(outfile, "td",
					     shared_p->available == 0 ? -NAN : (float)(100 *
										       shared_p->backups)
					     / shared_p->available);
			}
			end_tag(outfile, "tr");
		}
		end_tag(outfile, "tbody");
	}
	table_end(outfile);
	newsection(outfile, "Ranges");
	table_start(outfile, "r", "ranges");
	if (config.output_limit[0] & BIT1) {
		start_tag(outfile, "thead");
		start_tag(outfile, "tr");
		output_line(outfile, "th", "shared net name");
		output_line(outfile, "th", "first ip");
		output_line(outfile, "th", "last ip");
		output_line(outfile, "th", "max");
		output_line(outfile, "th", "cur");
		output_line(outfile, "th", "percent");
		output_line(outfile, "th", "touch");
		output_line(outfile, "th", "t+c");
		output_line(outfile, "th", "t+c perc");
		if (config.backups_found == true) {
			output_line(outfile, "th", "bu");
			output_line(outfile, "th", "bu perc");
		}
		end_tag(outfile, "tr");
		end_tag(outfile, "thead");
	}
	if (config.output_limit[1] & BIT1) {
		start_tag(outfile, "tbody");
		for (i = 0; i < num_ranges; i++) {
			start_tag(outfile, "tr");
			if (range_p->shared_net) {
				output_line(outfile, "td", range_p->shared_net->name);
			} else {
				output_line(outfile, "td", "not_defined");
			}
			output_line(outfile, "td", ntop_ipaddr(&range_p->first_ip));
			output_line(outfile, "td", ntop_ipaddr(&range_p->last_ip));
			output_double(outfile, "td", range_size);
			output_double(outfile, "td", range_p->count);
			output_float(outfile, "td", (float)(100 * range_p->count) / range_size);
			output_double(outfile, "td", range_p->touched);
			output_double(outfile, "td", range_p->touched + range_p->count);
			output_float(outfile, "td",
				     (float)(100 *
					     (range_p->touched + range_p->count)) / range_size);
			if (config.backups_found == true) {
				output_double(outfile, "td", range_p->backups);
				output_float(outfile, "td",
					     (float)(100 * range_p->backups) / range_size);
			}
			end_tag(outfile, "tr");
			range_p++;
			range_size = get_range_size(range_p);
		}
		end_tag(outfile, "tbody");
	}
	table_end(outfile);
	html_footer(outfile);
	if (outfile == stdout) {
		ret = fflush(stdout);
		if (ret) {
			error(0, 0, "output_html: fflush");
		}
	} else {
		ret = close_stream(outfile);
		if (ret) {
			error(0, 0, "output_html: fclose");
		}
	}
	return 0;
}

/*! \brief Output cvs format.
 * FIXME: This function should return void.
 */
int output_csv(void)
{
	unsigned int i;
	struct range_t *range_p;
	double range_size;
	struct shared_network_t *shared_p;
	FILE *outfile;
	int ret;

	if (config.output_file[0]) {
		outfile = fopen(config.output_file, "w+");
		if (outfile == NULL) {
			error(EXIT_FAILURE, errno, "output_csv: %s", config.output_file);
		}
	} else {
		outfile = stdout;
	}

	range_p = ranges;
	range_size = get_range_size(range_p);
	shared_p = shared_networks;
	if (config.output_limit[0] & BIT1) {
		fprintf(outfile, "\"Ranges:\"\n");
		fprintf
		    (outfile,
		     "\"shared net name\",\"first ip\",\"last ip\",\"max\",\"cur\",\"percent\",\"touch\",\"t+c\",\"t+c perc\"");
		if (config.backups_found == true) {
			fprintf(outfile, ",\"bu\",\"bu perc\"");
		}
		fprintf(outfile, "\n");
	}
	if (config.output_limit[1] & BIT1) {
		for (i = 0; i < num_ranges; i++) {
			if (range_p->shared_net) {
				fprintf(outfile, "\"%s\",", range_p->shared_net->name);
			} else {
				fprintf(outfile, "\"not_defined\",");
			}
			fprintf(outfile, "\"%s\",", ntop_ipaddr(&range_p->first_ip));
			fprintf(outfile,
				"\"%s\",\"%g\",\"%g\",\"%.3f\",\"%g\",\"%g\",\"%.3f\"",
				ntop_ipaddr(&range_p->last_ip), range_size,
				range_p->count,
				(float)(100 * range_p->count) / range_size,
				range_p->touched,
				range_p->touched + range_p->count,
				(float)(100 * (range_p->touched + range_p->count)) / range_size);
			if (config.backups_found == true) {
				fprintf(outfile, ",\"%g\",\"%.3f\"",
					range_p->backups,
					(float)(100 * range_p->backups) / range_size);
			}

			fprintf(outfile, "\n");
			range_p++;
			range_size = get_range_size(range_p);
		}
		fprintf(outfile, "\n");
	}
	if (config.output_limit[0] & BIT2) {
		fprintf(outfile, "\"Shared networks:\"\n");
		fprintf(outfile,
			"\"name\",\"max\",\"cur\",\"percent\",\"touch\",\"t+c\",\"t+c perc\"");
		if (config.backups_found == true) {
			fprintf(outfile, ",\"bu\",\"bu perc\"");
		}
		fprintf(outfile, "\n");
	}
	if (config.output_limit[1] & BIT2) {

		for (i = 0; i < num_shared_networks; i++) {
			shared_p++;
			fprintf(outfile,
				"\"%s\",\"%g\",\"%g\",\"%.3f\",\"%g\",\"%g\",\"%.3f\"",
				shared_p->name, shared_p->available,
				shared_p->used,
				shared_p->available == 0 ? -NAN : (float)(100 * shared_p->used) /
				shared_p->available, shared_p->touched,
				shared_p->touched + shared_p->used,
				shared_p->available == 0 ? -NAN : (float)(100 *
									  (shared_p->touched +
									   shared_p->used)) /
				shared_p->available);
			if (config.backups_found == true) {
				fprintf(outfile, ",\"%g\",\"%.3f\"",
					shared_p->backups,
					shared_p->available ==
					0 ? -NAN : (float)(100 * shared_p->backups) /
					shared_p->available);
			}

			fprintf(outfile, "\n");
		}
		fprintf(outfile, "\n");
	}
	if (config.output_limit[0] & BIT3) {
		fprintf(outfile, "\"Sum of all ranges:\"\n");
		fprintf(outfile,
			"\"name\",\"max\",\"cur\",\"percent\",\"touch\",\"t+c\",\"t+c perc\"");
		if (config.backups_found == true) {
			fprintf(outfile, ",\"bu\",\"bu perc\"");
		}
		fprintf(outfile, "\n");
	}
	if (config.output_limit[1] & BIT3) {

		fprintf(outfile,
			"\"%s\",\"%g\",\"%g\",\"%.3f\",\"%g\",\"%g\",\"%.3f\"",
			shared_networks->name, shared_networks->available,
			shared_networks->used,
			shared_networks->available ==
			0 ? -NAN : (float)(100 * shared_networks->used) /
			shared_networks->available, shared_networks->touched,
			shared_networks->touched + shared_networks->used,
			shared_networks->available ==
			0 ? -NAN : (float)(100 *
					   (shared_networks->touched +
					    shared_networks->used)) / shared_networks->available);
		if (config.backups_found == true) {
			fprintf(outfile, "%7g %8.3f",
				shared_networks->backups,
				shared_networks->available ==
				0 ? -NAN : (float)(100 * shared_networks->backups) /
				shared_networks->available);
		}
		fprintf(outfile, "\n");
	}
	if (outfile == stdout) {
		ret = fflush(stdout);
		if (ret) {
			error(0, 0, "output_cvs: fflush");
		}
	} else {
		ret = close_stream(outfile);
		if (ret) {
			error(0, 0, "output_cvs: fclose");
		}
	}
	return 0;
}

/*! \brief Output alarm text, and return program exit value.
 * FIXME: This function should return void.
 */
int output_alarming(void)
{
	FILE *outfile;
	struct range_t *range_p;
	double range_size;
	struct shared_network_t *shared_p;
	unsigned int i;
	float perc;
	int rw = 0, rc = 0, ro = 0, ri = 0, sw = 0, sc = 0, so = 0, si = 0;
	int ret_val, ret;

	range_p = ranges;
	range_size = get_range_size(range_p);
	shared_p = shared_networks;

	if (config.output_file[0]) {
		outfile = fopen(config.output_file, "w+");
		if (outfile == NULL) {
			error(EXIT_FAILURE, errno, "output_alarming: %s", config.output_file);
		}
	} else {
		outfile = stdout;
	}

	if (config.output_limit[1] & BIT1) {
		for (i = 0; i < num_ranges; i++) {
			if (config.snet_alarms && range_p->shared_net != shared_networks) {
				continue;
			}
			if (config.minsize < range_size) {
				perc = (float)(100 * range_p->count) / range_size;
				if (config.critical < perc && (range_size - range_p->count) < config.crit_count)
					rc++;
				else if (config.warning < perc && (range_size - range_p->count) < config.warn_count)
					rw++;
				else
					ro++;
			} else {
				ri++;
			}
			range_p++;
			range_size = get_range_size(range_p);
		}
	}
	if (config.output_limit[1] & BIT2) {
		for (i = 0; i < num_shared_networks; i++) {
			shared_p++;
			if (config.minsize < shared_p->available) {
				perc =
				    shared_p->available ==
				    0 ? 100 : (float)(100 * shared_p->used) / shared_p->available;
				if (config.critical < perc && shared_p->available < config.crit_count)
					sc++;
				else if (config.warning < perc && shared_p->available < config.warn_count)
					sw++;
				else
					so++;
			} else {
				si++;
			}
		}
	}

	if (sc || rc)
		ret_val = STATE_CRITICAL;
	else if (sw || rw)
		ret_val = STATE_WARNING;
	else
		ret_val = STATE_OK;

	if ((0 < rc && config.output_limit[1] & BIT1)
	    || (0 < sc && config.output_limit[1] & BIT2)) {
		fprintf(outfile, "CRITICAL: %s:", program_name);
	} else if ((0 < rw && config.output_limit[1] & BIT1)
		   || (0 < sw && config.output_limit[1] & BIT2)) {
		fprintf(outfile, "WARNING: %s:", program_name);
	} else {
		if (config.output_limit[1] & BIT3)
			fprintf(outfile, "OK:");
		else
			return ret_val;
	}
	if (config.output_limit[0] & BIT1) {
		fprintf(outfile, " Ranges - crit: %d warn: %d ok: %d", rc, rw, ro);
		if (ri != 0) {
			fprintf(outfile, " ignored: %d", ri);
		}
		fprintf(outfile, "; | range_crit=%d range_warn=%d range_ok=%d", rc, rw, ro);
		if (ri != 0) {
			fprintf(outfile, " range_ignored=%d", ri);
		}
		fprintf(outfile, "\n");
	} else {
		fprintf(outfile, " ");
	}
	if (config.output_limit[0] & BIT2) {
		fprintf(outfile, "Shared nets - crit: %d warn: %d ok: %d", sc, sw, so);
		if (si != 0) {
			fprintf(outfile, " ignored: %d", si);
		}
		fprintf(outfile, "; | snet_crit=%d snet_warn=%d snet_ok=%d", sc, sw, so);
		if (si != 0) {
			fprintf(outfile, " snet_ignored=%d\n", si);
		}
	}
	fprintf(outfile, "\n");
	if (outfile == stdout) {
		ret = fflush(stdout);
		if (ret) {
			error(0, 0, "output_alarming: fflush");
		}
	} else {
		ret = close_stream(outfile);
		if (ret) {
			error(0, 0, "output_alarming: fclose");
		}
	}
	return ret_val;
}

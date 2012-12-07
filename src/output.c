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

#include <config.h>

#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <langinfo.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

#include "close-stream.h"
#include "dhcpd-pools.h"
#include "strftime.h"

int output_txt(void)
{
	unsigned int i;
	struct range_t *range_p;
	unsigned long range_size;
	struct shared_network_t *shared_p;
	int ret;
	FILE *outfile;
	int max_ipaddr_length = dhcp_version == VERSION_6 ? 39 : 16;

	if (config.output_file[0]) {
		outfile = fopen(config.output_file, "w+");
		if (outfile == NULL) {
			err(EXIT_FAILURE, "output_txt: %s", config.output_file);
		}
	} else {
		outfile = stdout;
	}

	range_p = ranges;
	range_size = get_range_size(range_p);
	shared_p = shared_networks;

	if (config.output_limit[0] & output_limit_bit_1) {
		fprintf(outfile, "Ranges:\n");
		fprintf
		    (outfile,
		     "%-20s%-*s   %-*s %5s %5s %10s  %5s %5s %9s",
		     "shared net name",
		     max_ipaddr_length,
		     "first ip",
		     max_ipaddr_length,
		     "last ip",
		     "max", "cur", "percent", "touch", "t+c", "t+c perc");
		if (0 < num_backups) {
			fprintf(outfile, "     bu  bu perc");
		}
		fprintf(outfile, "\n");
	}
	if (config.output_limit[1] & output_limit_bit_1) {
		for (i = 0; i < num_ranges; i++) {
			if (range_p->shared_net) {
				fprintf(outfile, "%-20s",
					range_p->shared_net->name);
			} else {
				fprintf(outfile, "not_defined         ");
			}
			/* Outputting of first_ip and last_ip need to be
			 * separate since ntop_ipaddr always returns the
			 * same buffer */
			fprintf(outfile, "%-*s",
				max_ipaddr_length,
				ntop_ipaddr(&range_p->first_ip));
			fprintf(outfile,
				" - %-*s %5lu %5lu %10.3f  %5lu %5lu %9.3f",
				max_ipaddr_length,
				ntop_ipaddr(&range_p->last_ip),
				range_size,
				range_p->count,
				(float)(100 * range_p->count) / range_size,
				range_p->touched,
				range_p->touched + range_p->count,
				(float)(100 *
					(range_p->touched +
					 range_p->count)) / range_size);
			if (0 < num_backups) {
				fprintf(outfile, "%7lu %8.3f",
					range_p->backups,
					(float)(100 * range_p->backups) /
					range_size);
			}
			fprintf(outfile, "\n");
			range_p++;
			range_size = get_range_size(range_p);
		}
	}
	if (config.output_limit[1] & output_limit_bit_1
	    && config.output_limit[0] & output_limit_bit_2) {
		fprintf(outfile, "\n");
	}
	if (config.output_limit[0] & output_limit_bit_2) {
		fprintf(outfile, "Shared networks:\n");
		fprintf(outfile,
			"name                   max   cur     percent  touch    t+c  t+c perc");
		if (0 < num_backups) {
			fprintf(outfile, "     bu  bu perc");
		}
		fprintf(outfile, "\n");
	}
	if (config.output_limit[1] & output_limit_bit_2) {
		for (i = 0; i < num_shared_networks; i++) {
			shared_p++;
			fprintf(outfile,
				"%-20s %5lu %5lu %10.3f %7lu %6lu %9.3f",
				shared_p->name, shared_p->available,
				shared_p->used,
				(float)(100 * shared_p->used) /
				shared_p->available, shared_p->touched,
				shared_p->touched + shared_p->used,
				(float)(100 *
					(shared_p->touched +
					 shared_p->used)) /
				shared_p->available);
			if (0 < num_backups) {
				fprintf(outfile, "%7lu %8.3f",
					shared_p->backups,
					(float)(100 * shared_p->backups) /
					shared_p->available);
			}

			fprintf(outfile, "\n");
		}
	}
	if (config.output_limit[1] & output_limit_bit_2
	    && config.output_limit[0] & output_limit_bit_3) {
		fprintf(outfile, "\n");
	}
	if (config.output_limit[0] & output_limit_bit_3) {
		fprintf(outfile, "Sum of all ranges:\n");
		fprintf(outfile,
			"name                   max   cur     percent  touch    t+c  t+c perc");

		if (0 < num_backups) {
			fprintf(outfile, "     bu  bu perc");
		}
		fprintf(outfile, "\n");
	}
	if (config.output_limit[1] & output_limit_bit_3) {
		fprintf(outfile, "%-20s %5lu %5lu %10.3f %7lu %6lu %9.3f",
			shared_networks->name,
			shared_networks->available,
			shared_networks->used,
			(float)(100 * shared_networks->used) /
			shared_networks->available,
			shared_networks->touched,
			shared_networks->touched + shared_networks->used,
			(float)(100 *
				(shared_networks->touched +
				 shared_networks->used)) /
			shared_networks->available);

		if (0 < num_backups) {
			fprintf(outfile, "%7lu %8.3f",
				shared_networks->backups,
				(float)(100 * shared_networks->backups) /
				shared_networks->available);
		}
		fprintf(outfile, "\n");
	}
	if (outfile == stdout) {
		ret = fflush(stdout);
		if (ret) {
			warn("output_txt: fflush");
		}
	} else {
		ret = close_stream(outfile);
		if (ret) {
			warn("output_txt: fclose");
		}
	}

	return 0;
}

int output_xml(void)
{
	unsigned int i;
	struct range_t *range_p;
	unsigned long range_size;
	struct shared_network_t *shared_p;
	int ret;
	FILE *outfile;

	if (config.output_file[0]) {
		outfile = fopen(config.output_file, "w+");
		if (outfile == NULL) {
			err(EXIT_FAILURE, "output_xml: %s", config.output_file);
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
				fputs("</macaddress>\n</active_lease>\n",
				      outfile);
			}
		}
	}

	if (config.output_limit[1] & output_limit_bit_1) {
		for (i = 0; i < num_ranges; i++) {
			fprintf(outfile, "<subnet>\n");
			if (range_p->shared_net) {
				fprintf(outfile,
					"\t<location>%s</location>\n",
					range_p->shared_net->name);
			} else {
				fprintf(outfile, "\t<location></location>\n");
			}

			fprintf(outfile, "\t<network></network>\n");
			fprintf(outfile, "\t<netmask></netmask>\n");
			fprintf(outfile, "\t<range>%s ",
				ntop_ipaddr(&range_p->first_ip));
			fprintf(outfile, "- %s</range>\n",
				ntop_ipaddr(&range_p->last_ip));
			fprintf(outfile, "\t<gateway></gateway>\n");
			fprintf(outfile, "\t<defined>%lu</defined>\n",
				range_size);
			fprintf(outfile, "\t<used>%lu</used>\n",
				range_p->count);
			fprintf(outfile, "\t<free>%lu</free>\n",
				range_size - range_p->count);
			range_p++;
			range_size = get_range_size(range_p);
			fprintf(outfile, "</subnet>\n");
		}
	}

	if (config.output_limit[1] & output_limit_bit_2) {
		for (i = 0; i < num_shared_networks; i++) {
			shared_p++;
			fprintf(outfile, "<shared-network>\n");
			fprintf(outfile, "\t<location>%s</location>\n",
				shared_p->name);
			fprintf(outfile, "\t<defined>%lu</defined>\n",
				shared_p->available);
			fprintf(outfile, "\t<used>%lu</used>\n",
				shared_p->used);
			fprintf(outfile, "\t<free>%lu</free>\n",
				shared_p->available - shared_p->used);
			fprintf(outfile, "</shared-network>\n");
		}
	}

	if (config.output_limit[0] & output_limit_bit_3) {
		fprintf(outfile, "<summary>\n");
		fprintf(outfile, "\t<location>%s</location>\n",
			shared_networks->name);
		fprintf(outfile, "\t<defined>%lu</defined>\n",
			shared_networks->available);
		fprintf(outfile, "\t<used>%lu</used>\n", shared_networks->used);
		fprintf(outfile, "\t<free>%lu</free>\n",
			shared_networks->available - shared_networks->used);
		fprintf(outfile, "</summary>\n");
	}

	fprintf(outfile, "</dhcpstatus>\n");
	if (outfile == stdout) {
		ret = fflush(stdout);
		if (ret) {
			warn("output_xml: fflush");
		}
	} else {
		ret = close_stream(outfile);
		if (ret) {
			warn("output_xml: fclose");
		}
	}

	return 0;
}

int output_json(void)
{
	unsigned int i = 0;
	struct range_t *range_p;
	unsigned long range_size;
	struct shared_network_t *shared_p;
	int ret;
	FILE *outfile;
	unsigned int sep;

	if (config.output_file[0]) {
		outfile = fopen(config.output_file, "w+");
		if (outfile == NULL) {
			err(EXIT_FAILURE, "output_json: %s",
			    config.output_file);
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

	if (config.output_limit[1] & output_limit_bit_1) {
		if (sep) {
			fprintf(outfile, ",\n");
		}
		fprintf(outfile, "   \"subnets\": [\n");
		for (i = 0; i < num_ranges; i++) {
			fprintf(outfile, "         ");
			fprintf(outfile, "{ ");
			if (range_p->shared_net) {
				fprintf(outfile,
					"\"location\":\"%s\", ",
					range_p->shared_net->name);
			} else {
				fprintf(outfile, "\"location\":\"\", ");
			}

			fprintf(outfile, "\"range\":\"%s",
				ntop_ipaddr(&range_p->first_ip));
			fprintf(outfile, " - %s\", ",
				ntop_ipaddr(&range_p->last_ip));
			fprintf(outfile, "\"defined\":%lu, ", range_size);
			fprintf(outfile, "\"used\":%lu, ", range_p->count);
			fprintf(outfile, "\"free\":%lu ",
				range_size - range_p->count);
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

	if (config.output_limit[1] & output_limit_bit_2) {
		if (sep) {
			fprintf(outfile, ",\n");
		}
		fprintf(outfile, "   \"shared-networks\": [\n");
		for (i = 0; i < num_shared_networks; i++) {
			fprintf(outfile, "         ");
			shared_p++;
			fprintf(outfile, "{ ");
			fprintf(outfile, "\"location\":\"%s\", ",
				shared_p->name);
			fprintf(outfile, "\"defined\":%lu, ",
				shared_p->available);
			fprintf(outfile, "\"used\":%lu, ", shared_p->used);
			fprintf(outfile, "\"free\":%lu ",
				shared_p->available - shared_p->used);
			if (i + 1 < num_shared_networks)
				fprintf(outfile, "},\n");
			else
				fprintf(outfile, "}\n");
		}
		fprintf(outfile, "   ]");	/* end of shared-networks */
		sep++;
	}

	if (config.output_limit[0] & output_limit_bit_3) {
		if (sep) {
			fprintf(outfile, ",\n");
		}
		fprintf(outfile, "   \"summary\": {\n");
		fprintf(outfile, "         \"location\":\"%s\",\n",
			shared_networks->name);
		fprintf(outfile, "         \"defined\":%lu,\n",
			shared_networks->available);
		fprintf(outfile, "         \"used\":%lu,\n",
			shared_networks->used);
		fprintf(outfile, "         \"free\":%lu\n",
			shared_networks->available - shared_networks->used);
		fprintf(outfile, "   }");	/* end of summary */
		sep++;
	}

	fprintf(outfile, "\n}\n");
	if (outfile == stdout) {
		ret = fflush(stdout);
		if (ret) {
			warn("output_json: fflush");
		}
	} else {
		ret = close_stream(outfile);
		if (ret) {
			warn("output_json: fclose");
		}
	}

	return 0;
}

static void html_header(FILE *restrict f)
{
	char outstr[200];
	struct tm *tmp;

	struct stat statbuf;
	stat(config.dhcpdlease_file, &statbuf);

	tmp = localtime(&statbuf.st_mtime);
	if (tmp == NULL) {
		err(EXIT_FAILURE, "html_header: localtime");
	}
	if (strftime(outstr, sizeof(outstr), nl_langinfo(D_T_FMT), tmp) == 0) {
		errx(EXIT_FAILURE, "html_header: strftime returned 0");
	}

	fprintf(f, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \n");
	fprintf(f, "  \"http://www.w3.org/TR/html4/strict.dtd\">\n");
	fprintf(f, "<html>\n");
	fprintf(f, "<head>\n");
	fprintf(f, "<meta http-equiv=\"Content-Type\" ");
	fprintf(f, "content=\"text/html; charset=iso-8859-1\">\n");
	fprintf(f, "  <title>ISC dhcpd stats</title>\n");
	fprintf(f, "  <style  TYPE=\"text/css\">\n");
	fprintf(f, "  <!--\n");
	fprintf(f, "    table.dhcpd-pools {\n");
	fprintf(f, "      color: black;\n");
	fprintf(f, "      vertical-align: middle;\n");
	fprintf(f, "      text-align: center;\n");
	fprintf(f, "    }\n");
	fprintf(f, "    table.dhcpd-pools th.section {\n");
	fprintf(f, "      color: black;\n");
	fprintf(f, "      font-size: large;\n");
	fprintf(f, "      vertical-align: middle;\n");
	fprintf(f, "      text-align: left;\n");
	fprintf(f, "    }\n");
	fprintf(f, "    table.dhcpd-pools th.calign {\n");
	fprintf(f, "      color: black;\n");
	fprintf(f, "      vertical-align: middle;\n");
	fprintf(f, "      text-align: center;\n");
	fprintf(f, "      text-decoration: underline;\n");
	fprintf(f, "    }\n");
	fprintf(f, "    table.dhcpd-pools th.ralign {\n");
	fprintf(f, "      color: black;\n");
	fprintf(f, "      vertical-align: middle;\n");
	fprintf(f, "      text-align: right;\n");
	fprintf(f, "      text-decoration: underline;\n");
	fprintf(f, "    }\n");
	fprintf(f, "    table.dhcpd-pools td.calign {\n");
	fprintf(f, "      color: black;\n");
	fprintf(f, "      vertical-align: middle;\n");
	fprintf(f, "      text-align: center;\n");
	fprintf(f, "    }\n");
	fprintf(f, "    table.dhcpd-pools td.ralign {\n");
	fprintf(f, "      color: black;\n");
	fprintf(f, "      vertical-align: middle;\n");
	fprintf(f, "      text-align: right;\n");
	fprintf(f, "    }\n");
	fprintf(f, "    p.created {\n");
	fprintf(f, "      font-size: small;\n");
	fprintf(f, "      color: grey;\n");
	fprintf(f, "    }\n");
	fprintf(f, "    p.updated {\n");
	fprintf(f, "      font-size: small;\n");
	fprintf(f, "      color: grey;\n");
	fprintf(f, "      font-style: italic;\n");
	fprintf(f, "    }\n");
	fprintf(f, "  -->\n");
	fprintf(f, "  </style>\n");
	fprintf(f, "</head>\n");
	fprintf(f, "<body>\n");
	fprintf(f, "<a name=\"ranges\">The lease file mtime: %s</a>", outstr);
}

static void html_footer(FILE *restrict f)
{
	fprintf(f, "<p><br></p>\n");
	fprintf(f, "<hr>\n");
	fprintf(f, "<p class=created>\nData generated by ");
	fprintf(f, "<a href=\"%s\">", PACKAGE_URL);
	fprintf(f, "%s</a>.\n</p>\n", PACKAGE_STRING);
	fprintf(f, "<p class=updated>\n");
	fprintf(f, "<script type=\"text/javascript\">\n");
	fprintf(f, "  document.write(\"Last Updated On \" + ");
	fprintf(f, "document.lastModified + \".\")\n");
	fprintf(f, "</script>\n<br>\n</p>\n");
	fprintf(f, "</body>\n");
	fprintf(f, "</html>\n");
}

static void newrow(FILE *restrict f)
{
	fprintf(f, "<tr>\n");
}

static void endrow(FILE *restrict f)
{
	fprintf(f, "</tr>\n\n");
}

static void output_line(FILE *restrict f, char const *restrict type,
			char const *restrict class, char const *restrict text)
{
	fprintf(f, "  <%s class=%s>%s</%s>\n", type, class, text, type);
}

static void output_long(FILE *restrict f, char const *restrict type,
			unsigned long unlong)
{
	fprintf(f, "  <%s class=ralign>%lu</%s>\n", type, unlong, type);
}

static void output_float(FILE *f, char const *restrict type, float fl)
{
	fprintf(f, "  <%s class=ralign>%.3f</%s>\n", type, fl, type);
}

static void table_start(FILE *restrict f)
{
	fprintf(f, "<table width=\"75%%\" ");
	fprintf(f, "class=\"%s\" ", PACKAGE_NAME);
	fprintf(f, "summary=\"ISC dhcpd pool usage report\">\n");
}

static void table_end(FILE *restrict f)
{
	fprintf(f, "</table>\n");
}

static void newsection(FILE *restrict f, char const *restrict title)
{
	newrow(f);
	output_line(f, "td", "calign", "&nbsp;");
	endrow(f);
	newrow(f);
	output_line(f, "th", "section", title);
	endrow(f);
}

int output_html(void)
{
	unsigned int i;
	struct range_t *range_p;
	unsigned long range_size;
	struct shared_network_t *shared_p;
	int ret;
	FILE *outfile;
	if (config.output_file[0]) {
		outfile = fopen(config.output_file, "w+");
		if (outfile == NULL) {
			err(EXIT_FAILURE, "output_html: %s",
			    config.output_file);
		}
	} else {
		outfile = stdout;
	}

	range_p = ranges;
	range_size = get_range_size(range_p);
	shared_p = shared_networks;
	if (fullhtml) {
		html_header(outfile);
	}
	table_start(outfile);
	if (config.output_limit[0] & output_limit_bit_1) {
		newsection(outfile, "Ranges:");
		newrow(outfile);
		output_line(outfile, "th", "calign", "shared net name");
		output_line(outfile, "th", "calign", "first ip");
		output_line(outfile, "th", "calign", "last ip");
		output_line(outfile, "th", "ralign", "max");
		output_line(outfile, "th", "ralign", "cur");
		output_line(outfile, "th", "ralign", "percent");
		output_line(outfile, "th", "ralign", "touch");
		output_line(outfile, "th", "ralign", "t+c");
		output_line(outfile, "th", "ralign", "t+c perc");
		if (0 < num_backups) {
			output_line(outfile, "th", "ralign", "bu");
			output_line(outfile, "th", "ralign", "bu perc");
		}
		endrow(outfile);
	}
	if (config.output_limit[1] & output_limit_bit_1) {
		for (i = 0; i < num_ranges; i++) {
			newrow(outfile);
			if (range_p->shared_net) {
				output_line(outfile, "td", "calign",
					    range_p->shared_net->name);
			} else {
				output_line(outfile, "td", "calign",
					    "not_defined");
			}
			output_line(outfile, "td", "calign",
				    ntop_ipaddr(&range_p->first_ip));
			output_line(outfile, "td", "calign",
				    ntop_ipaddr(&range_p->last_ip));
			output_long(outfile, "td", range_size);
			output_long(outfile, "td", range_p->count);
			output_float(outfile, "td",
				     (float)(100 * range_p->count) /
				     range_size);
			output_long(outfile, "td", range_p->touched);
			output_long(outfile, "td",
				    range_p->touched + range_p->count);
			output_float(outfile, "td",
				     (float)(100 *
					     (range_p->touched +
					      range_p->count)) / range_size);
			if (0 < num_backups) {
				output_long(outfile, "td", range_p->backups);
				output_float(outfile, "td",
					     (float)(100 *
						     range_p->backups) /
					     range_size);
			}
			endrow(outfile);
			range_p++;
			range_size = get_range_size(range_p);
		}
	}
	table_end(outfile);
	table_start(outfile);
	if (config.output_limit[0] & output_limit_bit_2) {
		newsection(outfile, "Shared networks:");
		newrow(outfile);
		output_line(outfile, "th", "calign", "name");
		output_line(outfile, "th", "ralign", "max");
		output_line(outfile, "th", "ralign", "cur");
		output_line(outfile, "th", "ralign", "percent");
		output_line(outfile, "th", "ralign", "touch");
		output_line(outfile, "th", "ralign", "t+c");
		output_line(outfile, "th", "ralign", "t+c perc");
		if (0 < num_backups) {
			output_line(outfile, "th", "ralign", "bu");
			output_line(outfile, "th", "ralign", "bu perc");
		}
		endrow(outfile);
	}
	if (config.output_limit[1] & output_limit_bit_2) {
		for (i = 0; i < num_shared_networks; i++) {
			shared_p++;
			newrow(outfile);
			output_line(outfile, "td", "calign", shared_p->name);
			output_long(outfile, "td", shared_p->available);
			output_long(outfile, "td", shared_p->used);
			output_float(outfile, "td",
				     (float)(100 * shared_p->used) /
				     shared_p->available);
			output_long(outfile, "td", shared_p->touched);
			output_long(outfile, "td",
				    shared_p->touched + shared_p->used);
			output_float(outfile, "td",
				     (float)(100 *
					     (shared_p->touched +
					      shared_p->used)) /
				     shared_p->available);
			if (0 < num_backups) {
				output_long(outfile, "td", shared_p->backups);
				output_float(outfile, "td",
					     (float)(100 *
						     shared_p->backups) /
					     shared_p->available);
			}

			endrow(outfile);
		}
	}
	if (config.output_limit[0] & output_limit_bit_3) {
		newsection(outfile, "Sum of all ranges:");
		newrow(outfile);
		output_line(outfile, "th", "calign", "name");
		output_line(outfile, "th", "ralign", "max");
		output_line(outfile, "th", "ralign", "cur");
		output_line(outfile, "th", "ralign", "percent");
		output_line(outfile, "th", "ralign", "touch");
		output_line(outfile, "th", "ralign", "t+c");
		output_line(outfile, "th", "ralign", "t+c perc");
		if (0 < num_backups) {
			output_line(outfile, "th", "ralign", "bu");
			output_line(outfile, "th", "ralign", "bu perc");
		}

		endrow(outfile);
	}
	if (config.output_limit[1] & output_limit_bit_3) {
		newrow(outfile);
		output_line(outfile, "td", "calign", shared_networks->name);
		output_long(outfile, "td", shared_networks->available);
		output_long(outfile, "td", shared_networks->used);
		output_float(outfile, "td",
			     (float)(100 * shared_networks->used) /
			     shared_networks->available);
		output_long(outfile, "td", shared_networks->touched);
		output_long(outfile, "td",
			    shared_networks->touched + shared_networks->used);
		output_float(outfile, "td",
			     (float)(100 *
				     (shared_networks->touched +
				      shared_networks->used)) /
			     shared_networks->available);
		if (0 < num_backups) {
			output_long(outfile, "td", shared_networks->backups);
			output_float(outfile, "td",
				     (float)(100 *
					     shared_networks->backups) /
				     shared_networks->available);
		}
		endrow(outfile);
	}
	table_end(outfile);
	if (fullhtml) {
		html_footer(outfile);
	}
	if (outfile == stdout) {
		ret = fflush(stdout);
		if (ret) {
			warn("output_html: fflush");
		}
	} else {
		ret = close_stream(outfile);
		if (ret) {
			warn("output_html: fclose");
		}
	}
	return 0;
}

int output_csv(void)
{
	unsigned int i;
	struct range_t *range_p;
	unsigned long range_size;
	struct shared_network_t *shared_p;
	FILE *outfile;
	int ret;
	if (config.output_file[0]) {
		outfile = fopen(config.output_file, "w+");
		if (outfile == NULL) {
			err(EXIT_FAILURE, "output_csv: %s", config.output_file);
		}
	} else {
		outfile = stdout;
	}

	range_p = ranges;
	range_size = get_range_size(range_p);
	shared_p = shared_networks;
	if (config.output_limit[0] & output_limit_bit_1) {
		fprintf(outfile, "\"Ranges:\"\n");
		fprintf
		    (outfile,
		     "\"shared net name\",\"first ip\",\"last ip\",\"max\",\"cur\",\"percent\",\"touch\",\"t+c\",\"t+c perc\"");
		if (0 < num_backups) {
			fprintf(outfile, ",\"bu\",\"bu perc\"");
		}
		fprintf(outfile, "\n");
	}
	if (config.output_limit[1] & output_limit_bit_1) {
		for (i = 0; i < num_ranges; i++) {
			if (range_p->shared_net) {
				fprintf(outfile, "\"%s\",",
					range_p->shared_net->name);
			} else {
				fprintf(outfile, "\"not_defined\",");
			}
			fprintf(outfile, "\"%s\",",
				ntop_ipaddr(&range_p->first_ip));
			fprintf(outfile,
				"\"%s\",\"%lu\",\"%lu\",\"%.3f\",\"%lu\",\"%lu\",\"%.3f\"",
				ntop_ipaddr(&range_p->last_ip), range_size,
				range_p->count,
				(float)(100 * range_p->count) / range_size,
				range_p->touched,
				range_p->touched + range_p->count,
				(float)(100 *
					(range_p->touched +
					 range_p->count)) / range_size);
			if (0 < num_backups) {
				fprintf(outfile, ",\"%lu\",\"%.3f\"",
					range_p->backups,
					(float)(100 * range_p->backups) /
					range_size);
			}

			fprintf(outfile, "\n");
			range_p++;
			range_size = get_range_size(range_p);
		}
		fprintf(outfile, "\n");
	}
	if (config.output_limit[0] & output_limit_bit_2) {
		fprintf(outfile, "\"Shared networks:\"\n");
		fprintf(outfile,
			"\"name\",\"max\",\"cur\",\"percent\",\"touch\",\"t+c\",\"t+c perc\"");
		if (0 < num_backups) {
			fprintf(outfile, ",\"bu\",\"bu perc\"");
		}
		fprintf(outfile, "\n");
	}
	if (config.output_limit[1] & output_limit_bit_2) {

		for (i = 0; i < num_shared_networks; i++) {
			shared_p++;
			fprintf(outfile,
				"\"%s\",\"%lu\",\"%lu\",\"%.3f\",\"%lu\",\"%lu\",\"%.3f\"",
				shared_p->name, shared_p->available,
				shared_p->used,
				(float)(100 * shared_p->used) /
				shared_p->available, shared_p->touched,
				shared_p->touched + shared_p->used,
				(float)(100 *
					(shared_p->touched +
					 shared_p->used)) /
				shared_p->available);
			if (0 < num_backups) {
				fprintf(outfile, ",\"%lu\",\"%.3f\"",
					shared_p->backups,
					(float)(100 * shared_p->backups) /
					shared_p->available);
			}

			fprintf(outfile, "\n");
		}
		fprintf(outfile, "\n");
	}
	if (config.output_limit[0] & output_limit_bit_3) {
		fprintf(outfile, "\"Sum of all ranges:\"\n");
		fprintf(outfile,
			"\"name\",\"max\",\"cur\",\"percent\",\"touch\",\"t+c\",\"t+c perc\"");
		if (0 < num_backups) {
			fprintf(outfile, ",\"bu\",\"bu perc\"");
		}
		fprintf(outfile, "\n");
	}
	if (config.output_limit[1] & output_limit_bit_3) {

		fprintf(outfile,
			"\"%s\",\"%lu\",\"%lu\",\"%.3f\",\"%lu\",\"%lu\",\"%.3f\"",
			shared_networks->name, shared_networks->available,
			shared_networks->used,
			(float)(100 * shared_networks->used) /
			shared_networks->available,
			shared_networks->touched,
			shared_networks->touched + shared_networks->used,
			(float)(100 *
				(shared_networks->touched +
				 shared_networks->used)) /
			shared_networks->available);
		if (0 < num_backups) {
			fprintf(outfile, "%7lu %8.3f",
				shared_networks->backups,
				(float)(100 * shared_networks->backups) /
				shared_networks->available);
		}
		fprintf(outfile, "\n");
	}
	if (outfile == stdout) {
		ret = fflush(stdout);
		if (ret) {
			warn("output_cvs: fflush");
		}
	} else {
		ret = close_stream(outfile);
		if (ret) {
			warn("output_cvs: fclose");
		}
	}
	return 0;
}

int output_alarming(void)
{
	FILE *outfile;
	struct range_t *range_p;
	unsigned long range_size;
	struct shared_network_t *shared_p;
	unsigned int i;
	float perc;
	int rw = 0, rc = 0, ro = 0, sw = 0, sc = 0, so = 0;
	int ret_val, ret;

	range_p = ranges;
	range_size = get_range_size(range_p);
	shared_p = shared_networks;

	if (config.output_file[0]) {
		outfile = fopen(config.output_file, "w+");
		if (outfile == NULL) {
			err(EXIT_FAILURE, "output_alarming: %s",
			    config.output_file);
		}
	} else {
		outfile = stdout;
	}

	if (config.output_limit[1] & output_limit_bit_1) {
		for (i = 0; i < num_ranges; i++) {
			perc = (float)(100 * range_p->count) / range_size;
			if (config.critical < perc)
				rc++;
			else if (config.warning < perc)
				rw++;
			else
				ro++;
			range_p++;
			range_size = get_range_size(range_p);
		}
	}
	if (config.output_limit[1] & output_limit_bit_2) {
		for (i = 0; i < num_shared_networks; i++) {
			perc = (float)(100 * shared_p->used) /
			    shared_p->available;
			if (config.critical < perc)
				sc++;
			else if (config.warning < perc)
				sw++;
			else
				so++;
			shared_p++;
		}
	}
	if (0 < rc || 0 < sc) {
		ret_val = 2;
		fprintf(outfile, "CRITICAL: %s: ",
			program_invocation_short_name);
	} else if (0 < rw || 0 < sw) {
		ret_val = 1;
		fprintf(outfile, "WARNING: %s: ",
			program_invocation_short_name);
	} else {
		ret_val = 0;
		fprintf(outfile, "OK: ");
	}
	if (config.output_limit[0] & output_limit_bit_1) {
		fprintf(outfile, "Ranges; crit: %d warn: %d ok: %d ", rc, rw,
			ro);
	}
	if (config.output_limit[0] & output_limit_bit_2) {
		fprintf(outfile, "Shared nets; crit: %d warn: %d ok: %d", sc,
			sw, so);
	}
	fprintf(outfile, "\n");
	if (outfile == stdout) {
		ret = fflush(stdout);
		if (ret) {
			warn("output_alarming: fflush");
		}
	} else {
		ret = close_stream(outfile);
		if (ret) {
			warn("output_alarming: fclose");
		}
	}
	return ret_val;
}

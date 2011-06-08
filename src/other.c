/* http://dhcpd-pools.sourceforge.net/
** Copyright 2006- Sami Kerola <kerolasa@iki.fi>
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "dhcpd-pools.h"

#include <stdio.h>
#ifdef  HAVE_STDLIB_H
#include <stdlib.h>
#else				/* Not STDC_HEADERS */
extern void exit();
extern char *malloc();
#endif				/* STDC_HEADERS */
#include <err.h>
#include <errno.h>
#include <stddef.h>
#ifdef  HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

/* Simple memory allocation wrapper */
void *safe_malloc(const size_t size)
{
	void *ret = malloc(size);
	if (ret == NULL) {
		err(EXIT_FAILURE,
		    "safe_malloc: cannot allocate %lu bytes: ", size);
	}

	return ret;
}

/* Simple memory reallocation wrapper */
void *safe_realloc(void *ptr, const size_t size)
{
	void *ret = realloc(ptr, size);

	if (!ret && size)
		err(EXIT_FAILURE,
		    "safe_realloc: cannot allocate %zu bytes", size);
	return ret;
}

/* Simple strdup wrapper */
char *safe_strdup(const char *str)
{
	char *ret = strdup(str);

	if (!ret && str)
		err(EXIT_FAILURE, "cannot duplicate string");
	return ret;
}

void flip_ranges(struct range_t *ranges, struct range_t *tmp_ranges)
{
	unsigned int i = num_ranges - 1, j;

	for (j = 0; j < num_ranges; j++) {
		*(tmp_ranges + j) = *(ranges + i);
		i--;
	}

	memcpy(ranges, tmp_ranges, num_ranges * sizeof(struct range_t));
}

/* Free memory, flush buffers etc */
void clean_up(void)
{
	unsigned int i;

	/* Just in case there something in buffers */
	if (fflush(NULL)) {
		warn("clean_up: fflush");
	}
	num_shared_networks++;
	for (i = 0; i < num_shared_networks; i++) {
		free((shared_networks + i)->name);
	}
	free(config.dhcpdconf_file);
	free(config.dhcpdlease_file);
	free(config.output_file);
	free(ranges);
	free(leases);
	free(touches);
	free(shared_networks);
}

void print_version(void)
{
	fprintf(stdout, "%s\n", PACKAGE_STRING);
	fprintf(stdout,
		"Written by Sami Kerola.\nXML support by Dominic Germain, Sogetel inc.\n\n");
	fprintf(stdout,
		"License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n");
	fprintf(stdout,
		"This is free software: you are free to change and redistribute it.\n");
	fprintf(stdout,
		"There is NO WARRANTY, to the extent permitted by law.\n");
	exit(EXIT_SUCCESS);
}

void usage(int status)
{
	FILE *out;
	out = status != 0 ? stderr : stdout;

	fprintf(out, "\n\
Usage: %s [OPTIONS]\n\n", program_invocation_short_name);
	fprintf(out, "\
This is ISC dhcpd pools usage analyzer.\n\
\n");
	fprintf(out, "\
  -c, --config=FILE      path to the dhcpd.conf file\n\
  -l, --leases=FILE      path to the dhcpd.leases file\n\
  -f, --format=[thHcxX]  output format\n");
	fprintf(out, "\
                           t for text\n\
                           h for html table\n\
                           H for full html page\n\
                           x for xml\n\
                           X for xml with active lease details\n\
                           c for comma separated values\n");
	fprintf(out, "\
  -s, --sort=[nimcptTe]  sort ranges by\n\
                           n name\n\
                           i IP\n\
                           m maxium\n\
                           c current\n\
                           p percent\n\
                           t touched\n\
                           T t+c\n\
                           e t+c perc\n");
	fprintf(out, "\
  -r, --reverse		 reverse order sort\n\
  -o, --output=FILE      output into a file\n\
  -L, --limit=NR         output limit mask 77 - 00\n\
  -v, --version          version information\n\
  -h, --help             this screen\n\
\n\
Report bugs to <%s>\n\
Homepage: %s\n", PACKAGE_BUGREPORT, PACKAGE_URL);

	exit(out == stderr ? EXIT_FAILURE : EXIT_SUCCESS);
}

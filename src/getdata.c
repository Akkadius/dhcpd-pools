/*
** Copyright (C) 2006- Sami Kerola <	>
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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef  HAVE_STDLIB_H
#include <stdlib.h>
#else				/* Not STDC_HEADERS */
extern void exit();
extern char *malloc();
#define EXIT_FAILURE    1	/* Failing exit status.  */
#define EXIT_SUCCESS    0	/* Successful exit status.  */
#endif				/* STDC_HEADERS */

#ifdef  HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#define _XOPEN_SOURCE 600
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>

#include "dhcpd-pools.h"
#include "defaults.h"

/* Parse dhcpd.leases file. All performance boosts for this
 * function are wellcome */
int parse_leases(void)
{
	FILE *dhcpd_leases;
	char *line, *ipstring, *macstring, *macstring2;
	struct in_addr inp;
	struct stat lease_file_stats;
	unsigned long leasesmallocsize;
	unsigned long touchesmallocsize;
	unsigned long backupsmallocsize;
	int sw_active_lease = 0;

	num_touches = num_leases = num_backups = 0;

	dhcpd_leases = fopen(config.dhcpdlease_file, "r");
	if (dhcpd_leases == NULL) {
		eprintf("parse_leases: %s:", config.dhcpdlease_file);
		exit(EXIT_FAILURE);
	}
#ifdef POSIX_FADV_NOREUSE
	posix_fadvise((long) dhcpd_leases, 0, 0, POSIX_FADV_NOREUSE);
	if (errno) {
		eprintf("parse_leases: fadvise:");
		exit(EXIT_FAILURE);
	}
#endif				/* POSIX_FADV_NOREUSE */
#ifdef POSIX_FADV_SEQUENTIAL
	posix_fadvise((long) dhcpd_leases, 0, 0, POSIX_FADV_SEQUENTIAL);
	if (errno) {
		eprintf("parse_leases: fadvise:");
		exit(EXIT_FAILURE);
	}
#endif				/* POSIX_FADV_SEQUENTIAL */
	/* I found out that there's one lease address per 300 bytes in
	 * dhcpd.leases file. Malloc is little bit pessimistic and uses
	 * 250. If someone has higher density in lease file I'm
	 * interested to hear about that. */
	if (stat(config.dhcpdlease_file, &lease_file_stats)) {
		eprintf("parse_leases: %s:", config.dhcpdlease_file);
		exit(EXIT_FAILURE);
	}
	leasesmallocsize = (lease_file_stats.st_size / 250) + MAXLEN - 2;
	touchesmallocsize = (lease_file_stats.st_size / 250) + MAXLEN - 2;
	backupsmallocsize = (lease_file_stats.st_size / 120) + MAXLEN - 2;
	leases = safe_malloc(sizeof(long int) * leasesmallocsize);
	touches =
	    safe_malloc((size_t) sizeof(long int) * touchesmallocsize);

	line = safe_malloc(sizeof(long int) * MAXLEN);
	ipstring = safe_malloc(sizeof(long int) * MAXLEN);
	macstring = safe_malloc(sizeof(long int) * MAXLEN);
	macstring2 = safe_malloc(sizeof(long int) * MAXLEN);

	while (!feof(dhcpd_leases)) {
		fgets(line, MAXLEN, dhcpd_leases);
		/* It's a lease, save IP */
		if (strstr(line, "lease") == line) {
			strncpy(ipstring, line, (size_t) MAXLEN);
			nth_field(2, ipstring, ipstring);
			inet_aton(ipstring, &inp);
			sw_active_lease = 0;
		}
		/* Copy IP to correct array */
		else if (strstr(line, "binding state active")) {
			leases[num_leases] = htonl(inp.s_addr);
			num_leases++;
			sw_active_lease = 1;
		} else if (strstr(line, "  binding state free")) {
			touches[num_touches] = htonl(inp.s_addr);
			num_touches++;
		} else if (strstr(line, "  binding state backup")) {
			if (num_backups == 0) {
				backups =
				    safe_malloc((size_t) sizeof(long int) *
						backupsmallocsize);
			}
			backups[num_backups] = htonl(inp.s_addr);
			num_backups++;
		}

		if ((sw_active_lease == 1)
		    && (strstr(line, "hardware ethernet"))) {
			nth_field(3, macstring, line);
			macstring[strlen(macstring) - 1] = '\0';

			if (config.output_format[0] == 'X') {
				printf
				    ("<active_lease>\n\t<ip>%s</ip>\n\t<macaddress>%s</macaddress>\n</active_lease>\n",
				     ipstring, macstring);
			};
		}

		if ((num_leases > leasesmallocsize) ||
		    (num_touches > touchesmallocsize) ||
		    (num_backups > backupsmallocsize)) {
			printf("WARNING: running out of memory\n");
			printf("\tlease/touch/backup = %lu/%lu/%lu\n",
			       leasesmallocsize, touchesmallocsize,
			       backupsmallocsize);
			printf("\tlease/touch/backup = %lu/%lu/%lu\n",
			       num_leases, num_touches, num_backups);
			printf
			    ("Code should realloc() and init new memory, but no time to write that now!\n");
			exit(EXIT_FAILURE);
		}
	}

	return 0;
}

/* Like strcpy but for field which is separated by white spaces.
 * Number of first field is 1 and not 0 like C programs should
 * have. Question of semantics, send mail to author if this
 * annoys. All performance boosts for this function are well
 * come. */
int nth_field(int n, char *dest, const char *src)
{
	int i, j = 0, wordn = 0, len;

	len = strlen(src);

	for (i = 0; i < len; i++) {
		if (isspace(src[i])) {
			if (!(wordn < n)) {
				dest[j] = '\0';
				break;
			}
			j = 0;
		} else {
			if (j == 0) {
				wordn++;
			}
			if (wordn == n) {
				dest[j] = src[i];
			}
			j++;
		}
	}
	return 0;
}

/* dhcpd.conf interesting words */
int is_interesting_config_clause(char *s)
{
	if (strstr(s, "shared-network")) {
		return 1;
	} else if (strstr(s, "range")) {
		return 3;
	} else if (strstr(s, "include")) {
		return 4;
	} else {
		return 0;
	}
}

/* TODO: This spagetti monster function need to be rewrote at
 * least ones. */
char *parse_config(int is_include, char *config_file,
		   char *current_shared_name,
		   char *next_free_shared_name,
		   struct shared_network_t *shared_p)
{
	FILE *dhcpd_config;
	int i = 0, newclause = true, argument = false, comment =
	    false, braces = 0, quote = false;
	char *word, c;
	int braces_shared = 1000;
	struct in_addr inp;
	struct range_t *range_p;

	char *last_shared_name;
	last_shared_name = SHARED_NETWORKS_NAMES + shared_net_names;

	word = safe_malloc(sizeof(char) * MAXLEN);

	if (is_include) {
		/* Default place holder for ranges "All networks". */
		shared_p->name = current_shared_name;
	}

	/* Open configuration file */
	dhcpd_config = fopen(config_file, "r");
	if (dhcpd_config == NULL) {
		eprintf("parse_config: %s:", config_file);
		exit(EXIT_FAILURE);
	}
#ifdef POSIX_FADV_NOREUSE
	posix_fadvise((long) dhcpd_config, 0, 0, POSIX_FADV_NOREUSE);
	if (errno) {
		eprintf("parse_config: fadvise:");
		exit(EXIT_FAILURE);
	}
#endif				/* POSIX_FADV_NOREUSE */
#ifdef POSIX_FADV_SEQUENTIAL
	posix_fadvise((long) dhcpd_config, 0, 0, POSIX_FADV_SEQUENTIAL);
	if (errno) {
		eprintf("parse_config: fadvise:");
		exit(EXIT_FAILURE);
	}
#endif				/* POSIX_FADV_SEQUENTIAL */
	/* Very hairy stuff begins. */
	while (!feof(dhcpd_config)) {
		c = fgetc(dhcpd_config);
		/* Certain characters are magical */
		switch (c) {
			/* Handle comments if they are not quoted */
		case '#':
			if (quote == false) {
				comment = true;
			}
			continue;
		case '"':
			if (comment == false) {
				quote++;
				/* Either one or zero */
				quote = quote % 2;
			}
			continue;
		case '\n':
			/* New line resets comment section, but not if quoted */
			if (quote == false) {
				comment = false;
			}
			break;
		case ';':
			/* Quoted colon does not mean new clause */
			if (quote == true) {
				break;
			}
			if (comment == false && argument != 2
			    && argument != 4) {
				newclause = true;
				i = 0;
			} else if (argument == 2) {
				/* Range ends to ; and this hair in code make two
				 * ranges wrote to gether like...
				 *
				 * range 10.20.30.40 10.20.30.41;range 10.20.30.42 10.20.30.43;
				 * 
				 * ...to be interpreted correctly. */
				c = ' ';
			}
			continue;
		case '{':
			if (quote == true) {
				break;
			}
			if (comment == false) {
				braces++;
			}
			/* i == 0 detects word that ends to brace like:
			 *
			 * shared-network DSL{ ... */
			if (i == 0) {
				newclause = true;
			}
			continue;
		case '}':
			if (quote == true) {
				break;
			}
			if (comment == false) {
				braces--;
				/* End of shared-network */
				if (braces_shared == braces) {
					current_shared_name =
					    shared_net_names;
					/* TODO: Using 1000 is lame, but works. */
					braces_shared = 1000;
					shared_p = shared_networks;
				}
				/* Not literally true, but works for this program */
				newclause = true;
			}
			continue;
		default:
			break;
		}

		/* Either inside comment or Nth word of clause. */
		if (comment == true
		    || (newclause == false && argument == 0)) {
			continue;
		}
		/* Strip white spaces before new clause word. */
		if ((newclause == true || argument != 0) && isspace(c)
		    && i == 0) {
			continue;
		}
		/* Save to word which clause this is. */
		if ((newclause == true || argument != 0)
		    && (!isspace(c) || quote == true)) {
			word[i] = c;
			i++;
			/* Long word which is almost causing overflow. Not any of words
			 * this program is looking for are this long. */
			if (i > MAXLEN) {
				newclause = false;
				i = 0;
				continue;
			}
		}
		/* See if clause is something that parser is looking for. */
		else if (newclause == true) {
			/* Insert string end & set state */
			word[i] = '\0';
			newclause = false;
			i = 0;

			argument = is_interesting_config_clause(word);
		}
		/* words after range, shared-network or include */
		else if (argument != 0) {
			word[i] = '\0';
			newclause = false;
			i = 0;

			switch (argument) {
			case 0:
				/* printf ("nothing interesting: %s\n", word); */
				argument = 0;
				break;
			case 1:
				/* printf ("shared-network named: %s\n", word); */
				strcpy(next_free_shared_name, word);
				shared_p =
				    shared_networks + num_shared_networks;
				num_shared_networks++;
				shared_p++;
				shared_p->name = next_free_shared_name;
				shared_p->available = 0;
				shared_p->used = 0;
				shared_p->touched = 0;
				shared_p->backups = 0;
				/* Temporary abuse of argument variable */
				argument =
				    strlen(next_free_shared_name) + 1;
				if (last_shared_name >
				    next_free_shared_name + argument) {
					next_free_shared_name += argument;
				} else {
					/* TODO: make this go away by reallocationg more space. */
					eprintf
					    ("parse_config: End of shared-network space, increase SHARED_NETWORKS_NAMES and recompile");
					exit(EXIT_FAILURE);
				}
				argument = 0;
				braces_shared = braces;
				break;
			case 2:
				/* printf ("range 2nd ip: %s\n", word); */
				range_p = ranges + num_ranges;
				inet_aton(word, &inp);
				argument = 0;
				range_p->last_ip = htonl(inp.s_addr) + 1;
				range_p->count = 0;
				range_p->touched = 0;
				range_p->backups = 0;
				range_p->shared_net = shared_p;
				num_ranges++;
				if (num_ranges > RANGES) {
					eprintf
					    ("parse_config: Range space full! Increase RANGES and recompile.");
					exit(EXIT_FAILURE);
				}
				newclause = true;
				break;
			case 3:
				/* printf ("range 1nd ip: %s\n", word); */
				range_p = ranges + num_ranges;
				inet_aton(word, &inp);
				range_p->first_ip = htonl(inp.s_addr) - 1;
				if (range_p->first_ip == UINT_MAX) {
					/* word was not ip, try again */
					break;
				}
				argument = 2;
				break;
			case 4:
				/* printf ("include file: %s\n", word); */
				argument = 0;
				next_free_shared_name =
				    parse_config(false, word, current_shared_name,
						 next_free_shared_name,
						 shared_p);
				newclause = true;
				break;
			default:
				eprintf
				    ("parse_config: This cannot happen, report a bug!");
				exit(EXIT_FAILURE);
			}
		}
	}
	free(word);
	return next_free_shared_name;
}

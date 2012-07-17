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

#ifdef  HAVE_STDLIB_H
#include <stdlib.h>
#else				/* Not STDC_HEADERS */
extern char *malloc();
#define EXIT_FAILURE    1	/* Failing exit status.  */
#define EXIT_SUCCESS    0	/* Successful exit status.  */
#endif				/* STDC_HEADERS */

#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

#include "defaults.h"
#include "dhcpd-pools.h"

/* Parse dhcpd.leases file. All performance boosts for this function are
 * wellcome */
int parse_leases(void)
{
	FILE *dhcpd_leases;
	char *line, *ipstring, *macstring = NULL;
	struct in_addr inp;
	struct stat lease_file_stats;
	struct macaddr_t *macaddr_p = NULL;
	int sw_active_lease = 0;
	struct leases_t *lease;

	num_touches = num_leases = num_backups = 0;

	dhcpd_leases = fopen(config.dhcpdlease_file, "r");
	if (dhcpd_leases == NULL) {
		err(EXIT_FAILURE, "parse_leases: %s", config.dhcpdlease_file);
	}
#ifdef POSIX_FADV_WILLNEED
	posix_fadvise(fileno(dhcpd_leases), 0, 0, POSIX_FADV_WILLNEED);
	if (errno) {
		err(EXIT_FAILURE, "parse_leases: fadvise %s",
		    config.dhcpdlease_file);
	}
#endif				/* POSIX_FADV_WILLNEED */
#ifdef POSIX_FADV_SEQUENTIAL
	posix_fadvise(fileno(dhcpd_leases), 0, 0, POSIX_FADV_SEQUENTIAL);
	if (errno) {
		err(EXIT_FAILURE, "parse_leases: fadvise %s",
		    config.dhcpdlease_file);
	}
#endif				/* POSIX_FADV_SEQUENTIAL */

	/* I found out that there's one lease address per 300 bytes in
	 * dhcpd.leases file. Malloc is little bit pessimistic and uses 250.
	 * If someone has higher density in lease file I'm interested to
	 * hear about that. */
	if (stat(config.dhcpdlease_file, &lease_file_stats)) {
		err(EXIT_FAILURE, "parse_leases: %s", config.dhcpdlease_file);
	}

	line = safe_malloc(sizeof(char) * MAXLEN);
	ipstring = safe_malloc(sizeof(char) * MAXLEN);
	if (config.output_format[0] == 'X') {
		macstring = safe_malloc(sizeof(char) * 18);
		macaddr = safe_malloc(sizeof(struct macaddr_t));
		macaddr_p = macaddr;
		macaddr_p->next = NULL;
	}

	while (!feof(dhcpd_leases)) {
		if (!fgets(line, MAXLEN, dhcpd_leases) && ferror(dhcpd_leases)) {
			err(EXIT_FAILURE, "parse_leases: %s",
			    config.dhcpdlease_file);
		}
		/* It's a lease, save IP */
		if (xstrstr(line, "lease", 5)) {
			strncpy(ipstring, line, MAXLEN);
			nth_field(2, ipstring, ipstring);
			inet_aton(ipstring, &inp);
			sw_active_lease = 0;
		}
		/* Copy IP to correct array */
		else if (xstrstr(line, "  binding state active", 22)) {
			/* remove old entry, if exists */
			if ((lease = find_lease(htonl(inp.s_addr))) != NULL) {
				delete_lease(lease);
			}
			add_lease(htonl(inp.s_addr), ACTIVE);
			sw_active_lease = 1;
		} else if (xstrstr(line, "  binding state free", 20)) {
			/* remove old entry, if exists */
			if ((lease = find_lease(htonl(inp.s_addr))) != NULL) {
				delete_lease(lease);
			}
			add_lease(htonl(inp.s_addr), FREE);
		} else if (xstrstr(line, "  binding state backup", 22)) {
			/* remove old entry, if exists */
			if ((lease = find_lease(htonl(inp.s_addr))) != NULL) {
				delete_lease(lease);
			}
			add_lease(htonl(inp.s_addr), BACKUP);
		}

		if ((macaddr != NULL)
		    && (sw_active_lease == 1)
		    && (xstrstr(line, "  hardware ethernet", 19))) {
			nth_field(3, macstring, line);
			if (macstring) {
				macstring[17] = '\0';
				macaddr_p->ethernet = safe_strdup(macstring);
				macaddr_p->ip = safe_strdup(ipstring);
				macaddr_p->next =
				    safe_malloc(sizeof(struct macaddr_t));
				macaddr_p = macaddr_p->next;
				macaddr_p->next = NULL;
			}
		}
	}
	free(line);
	free(ipstring);
	if (macaddr != NULL) {
		free(macstring);
	}
	fclose(dhcpd_leases);
	return 0;
}

/* Like strcpy but for field which is separated by white spaces. Number of
 * first field is 1 and not 0 like C programs should have. Question of
 * semantics, send mail to author if this annoys. All performance boosts for
 * this function are well come. */
int nth_field(int n, char *restrict dest, const char *restrict src)
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
static int is_interesting_config_clause(char const *restrict s)
{
	if (strstr(s, "range")) {
		return 3;
	} else if (strstr(s, "shared-network")) {
		return 1;
	} else if (strstr(s, "include")) {
		return 4;
	} else {
		return 0;
	}
}

/* FIXME: This spagetti monster function need to be rewrote at least ones. */
void parse_config(int is_include, const char *restrict config_file,
		  struct shared_network_t *restrict shared_p)
{
	FILE *dhcpd_config;
	bool newclause = true, comment = false;
	int quote = 0, braces = 0, argument = 0;
	size_t i = 0;
	char *word, c;
	int braces_shared = 1000;
	struct in_addr inp;
	struct range_t *range_p;

	word = safe_malloc(sizeof(char) * MAXLEN);

	if (is_include) {
		/* Default place holder for ranges "All networks". */
		shared_p->name = shared_networks->name;
	}

	/* Open configuration file */
	dhcpd_config = fopen(config_file, "r");
	if (dhcpd_config == NULL) {
		err(EXIT_FAILURE, "parse_config: %s", config_file);
	}
#ifdef POSIX_FADV_WILLNEED
	posix_fadvise(fileno(dhcpd_config), 0, 0, POSIX_FADV_WILLNEED);
	if (errno) {
		err(EXIT_FAILURE, "parse_config: fadvise %s", config_file);
	}
#endif				/* POSIX_FADV_WILLNEED */
#ifdef POSIX_FADV_SEQUENTIAL
	posix_fadvise(fileno(dhcpd_config), 0, 0, POSIX_FADV_SEQUENTIAL);
	if (errno) {
		err(EXIT_FAILURE, "parse_config: fadvise %s", config_file);
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
			/* New line resets comment section, but
			 * not if quoted */
			if (quote == false) {
				comment = false;
			}
			break;
		case ';':
			/* Quoted colon does not mean new clause */
			if (0 < quote) {
				break;
			}
			if (comment == false && argument != 2 && argument != 4) {
				newclause = true;
				i = 0;
			} else if (argument == 2) {
				/* Range ends to ; and this hair in code
				 * make two ranges wrote to gether like...
				 *
				 * range 10.20.30.40 10.20.30.41;range 10.20.30.42 10.20.30.43;
				 *
				 * ...to be interpreted correctly. */
				c = ' ';
			}
			continue;
		case '{':
			if (0 < quote) {
				break;
			}
			if (comment == 0) {
				braces++;
			}
			/* i == 0 detects word that ends to brace like:
			 *
			 * shared-network DSL{ ... */
			if (i == 0) {
				newclause = true;
				continue;
			} else {
				break;
			}
		case '}':
			if (0 < quote) {
				break;
			}
			if (comment == false) {
				braces--;
				/* End of shared-network */
				if (braces_shared == braces) {
					/* FIXME: Using 1000 is lame, but
					 * works. */
					braces_shared = 1000;
					shared_p = shared_networks;
				}
				/* Not literally true, but works for this
				 * program */
				newclause = true;
			}
			continue;
		default:
			break;
		}

		/* Either inside comment or Nth word of clause. */
		if (comment == true || (newclause == false && argument == 0)) {
			continue;
		}
		/* Strip white spaces before new clause word. */
		if ((newclause == true || argument != 0) && isspace(c)
		    && i == 0) {
			continue;
		}
		/* Save to word which clause this is. */
		if ((newclause == true || argument != 0)
		    && (!isspace(c) || 0 < quote)) {
			word[i] = c;
			i++;
			/* Long word which is almost causing overflow. None
			 * of words are this long which the program is
			 * searching. */
			if (MAXLEN < i) {
				newclause = false;
				i = 0;
				continue;
			}
		}
		/* See if clause is something that parser is looking for. */
		else if (newclause == true) {
			/* Insert string end & set state */
			word[i] = '\0';
			if (word[i - 1] != '{') {
				newclause = false;
			}
			i = 0;
			argument = is_interesting_config_clause(word);
		}
		/* words after range, shared-network or include */
		else if (argument != 0) {
			word[i] = '\0';
			newclause = false;
			i = 0;

			switch (argument) {
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
				if (RANGES < num_ranges + 1) {
					RANGES *= 2;
					ranges =
					    safe_realloc(ranges,
							 sizeof(struct
								range_t) *
							 RANGES);
					range_p = ranges + num_ranges;
				}
				newclause = true;
				break;
			case 3:
				/* printf ("range 1nd ip: %s\n", word); */
				range_p = ranges + num_ranges;
				if (!(inet_aton(word, &inp))) {
					/* word was not ip, try
					 * again */
					break;
				}
				range_p->first_ip = htonl(inp.s_addr) - 1;
				argument = 2;
				break;
			case 1:
				/* printf ("shared-network named: %s\n", word); */
				num_shared_networks++;
				shared_p =
				    shared_networks + num_shared_networks;
				shared_p->name = safe_strdup(word);
				shared_p->available = 0;
				shared_p->used = 0;
				shared_p->touched = 0;
				shared_p->backups = 0;
				if (SHARED_NETWORKS < num_shared_networks + 2) {
					/* FIXME: make this
					 * away by reallocationg
					 * more space. */
					errx(EXIT_FAILURE,
					     "parse_config: increase default.h SHARED_NETWORKS and recompile");
				}
				argument = 0;
				braces_shared = braces;
				break;
			case 4:
				/* printf ("include file: %s\n", word); */
				argument = 0;
				parse_config(false, word, shared_p);
				newclause = true;
				break;
			case 0:
				/* printf ("nothing interesting: %s\n", word); */
				argument = 0;
				break;
			default:
				warnx("impossible occurred, report a bug");
				assert(0);
			}
		}
	}
	free(word);
	fclose(dhcpd_config);
	return;
}

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

/*! \file getdata.c
 * \brief Functions to read data from dhcpd.conf and dhcdp.leases files.
 */

#include <config.h>

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "error.h"
#include "xalloc.h"

#include "dhcpd-pools.h"
#include "defaults.h"

/*! \brief Lease file parser.  The parser can only read ISC DHCPD
 * dhcpd.leases file format.  */
int parse_leases(void)
{
	FILE *dhcpd_leases;
	char *line, *ipstring, macstring[20], *stop;
	union ipaddr_t addr;
	struct stat lease_file_stats;
	int ethernets = 0;	/* boolean */
	struct leases_t *lease;

	dhcpd_leases = fopen(config.dhcpdlease_file, "r");
	if (dhcpd_leases == NULL)
		error(EXIT_FAILURE, errno, "parse_leases: %s", config.dhcpdlease_file);
#ifdef HAVE_POSIX_FADVISE
# ifdef POSIX_FADV_SEQUENTIAL
	if (posix_fadvise(fileno(dhcpd_leases), 0, 0, POSIX_FADV_SEQUENTIAL) != 0)
		error(EXIT_FAILURE, errno, "parse_leases: fadvise %s", config.dhcpdlease_file);
# endif				/* POSIX_FADV_SEQUENTIAL */
#endif				/* HAVE_POSIX_FADVISE */
	/* I found out that there's one lease address per 300 bytes in
	 * dhcpd.leases file. Malloc is little bit pessimistic and uses 250.
	 * If someone has higher density in lease file I'm interested to
	 * hear about that. */
	if (stat(config.dhcpdlease_file, &lease_file_stats))
		error(EXIT_FAILURE, errno, "parse_leases: %s", config.dhcpdlease_file);
	line = xmalloc(sizeof(char) * MAXLEN);
	line[0] = '\0';
	ipstring = xmalloc(sizeof(char) * MAXLEN);
	ipstring[0] = '\0';
	if (config.print_mac_addreses == 1)
		ethernets = 1;
	while (!feof(dhcpd_leases)) {
		if (!fgets(line, MAXLEN, dhcpd_leases) && ferror(dhcpd_leases))
			error(EXIT_FAILURE, errno, "parse_leases: %s", config.dhcpdlease_file);
		switch (xstrstr(line)) {
			/* It's a lease, save IP */
		case PREFIX_LEASE:
			stop =
			    memccpy(ipstring,
				    line + (config.ip_version ==
					    IPv4 ? 6 : 9), ' ', strlen(line));
			if (stop != NULL) {
				--stop;
				*stop = '\0';
			}
			parse_ipaddr(ipstring, &addr);
			break;
		case PREFIX_BINDING_STATE_FREE:
		case PREFIX_BINDING_STATE_ABANDONED:
		case PREFIX_BINDING_STATE_EXPIRED:
		case PREFIX_BINDING_STATE_RELEASED:
			if ((lease = find_lease(&addr)) != NULL)
				delete_lease(lease);
			add_lease(&addr, FREE);
			break;
		case PREFIX_BINDING_STATE_ACTIVE:
			/* remove old entry, if exists */
			if ((lease = find_lease(&addr)) != NULL)
				delete_lease(lease);
			add_lease(&addr, ACTIVE);
			break;
		case PREFIX_BINDING_STATE_BACKUP:
			/* remove old entry, if exists */
			if ((lease = find_lease(&addr)) != NULL)
				delete_lease(lease);
			add_lease(&addr, BACKUP);
			config.backups_found = 1;
			break;
		case PREFIX_HARDWARE_ETHERNET:
			if (ethernets == 0)
				break;
			memcpy(macstring, line + 20, 17);
			macstring[17] = '\0';
			if ((lease = find_lease(&addr)) != NULL)
				lease->ethernet = xstrdup(macstring);
			break;
		default:
			/* do nothing */ ;
		}
	}
#undef HAS_PREFIX
	free(line);
	free(ipstring);
	fclose(dhcpd_leases);
	return 0;
}

/*! \brief Keyword search in dhcpd.conf file.
 * \param s A line from the dhcpd.conf file.
 * \return Indicator what configuration was found. */
static int is_interesting_config_clause(char const *restrict s)
{
	if (strstr(s, "range"))
		return ITS_A_RANGE_FIRST_IP;
	if (strstr(s, "shared-network"))
		return ITS_A_SHAREDNET;
	if (strstr(s, "include"))
		return ITS_AN_INCLUCE;
	return ITS_NOTHING_INTERESTING;
}

/*! \brief Flip first and last IP in range if they are in unusual order.
 */
static void reorder_last_first(struct range_t *range_p)
{
	if (ipcomp(&range_p->first_ip, &range_p->last_ip) > 0) {
		union ipaddr_t tmp;

		tmp = range_p->first_ip;
		range_p->first_ip = range_p->last_ip;
		range_p->last_ip = tmp;
	}
}

/*! \brief The dhcpd.conf file parser.
 * FIXME: This spaghetti monster function need to be rewrote at least
 * ones.
 */
void parse_config(int is_include, const char *restrict config_file,
		  struct shared_network_t *restrict shared_p)
{
	FILE *dhcpd_config;
	int newclause = 1, comment = 0, one_ip_range = 0; /* booleans */
	int quote = 0, braces = 0, argument = ITS_NOTHING_INTERESTING;
	size_t i = 0;
	char *word;
	int braces_shared = 1000;
	union ipaddr_t addr;
	struct range_t *range_p = NULL;

	word = xmalloc(sizeof(char) * MAXLEN);
	if (is_include)
		/* Default place holder for ranges "All networks". */
		shared_p->name = shared_networks->name;
	/* Open configuration file */
	dhcpd_config = fopen(config_file, "r");
	if (dhcpd_config == NULL)
		error(EXIT_FAILURE, errno, "parse_config: %s", config_file);
#ifdef HAVE_POSIX_FADVISE
# ifdef POSIX_FADV_SEQUENTIAL
	if (posix_fadvise(fileno(dhcpd_config), 0, 0, POSIX_FADV_SEQUENTIAL) != 0)
		error(EXIT_FAILURE, errno, "parse_config: fadvise %s", config_file);
# endif				/* POSIX_FADV_SEQUENTIAL */
#endif				/* HAVE_POSIX_FADVISE */
	/* Very hairy stuff begins. */
	while (unlikely(!feof(dhcpd_config))) {
		char c;

		c = fgetc(dhcpd_config);
		/* Certain characters are magical */
		switch (c) {
			/* Handle comments if they are not quoted */
		case '#':
			if (quote == 0)
				comment = 1;
			continue;
		case '"':
			if (comment == 0) {
				quote++;
				/* Either one or zero */
				quote = quote % 2;
			}
			continue;
		case '\n':
			/* New line resets comment section, but
			 * not if quoted */
			if (quote == 0)
				comment = 0;
			break;
		case ';':
			/* Quoted colon does not mean new clause */
			if (0 < quote)
				break;
			if (comment == 0
			    && argument != ITS_A_RANGE_FIRST_IP
			    && argument != ITS_A_RANGE_SECOND_IP && argument != ITS_AN_INCLUCE) {
				newclause = 1;
				i = 0;
			} else if (argument == ITS_A_RANGE_FIRST_IP && one_ip_range == 1) {
				argument = ITS_A_RANGE_SECOND_IP;
				c = ' ';
			} else if (argument == ITS_A_RANGE_SECOND_IP && 0 < i) {
				/* Range ends to ; and this hair in code
				 * make two ranges wrote together like...
				 *
				 * range 10.20.30.40 10.20.30.41;range 10.20.30.42 10.20.30.43;
				 *
				 * ...to be interpreted correctly. */
				c = ' ';
				break;
			} else if (argument == ITS_A_RANGE_SECOND_IP && i == 0) {
				if (!range_p) {
					fpos_t pos;
					fgetpos(dhcpd_config, &pos);
					error(EXIT_FAILURE, 0, "parse_config: parsing failed at position: %Ld", pos);
				}
				range_p->last_ip = range_p->first_ip;
				goto newrange;
			}
			continue;
		case '{':
			if (0 < quote)
				break;
			if (comment == 0)
				braces++;
			/* i == 0 detects word that ends to brace like:
			 *
			 * shared-network DSL{ ... */
			if (i == 0) {
				newclause = 1;
				continue;
			} else
				break;
		case '}':
			if (0 < quote)
				break;
			if (comment == 0) {
				braces--;
				/* End of shared-network */
				if (braces_shared == braces) {
					/* FIXME: Using 1000 is lame, but
					 * works. */
					braces_shared = 1000;
					shared_p = shared_networks;
				}
				/* Not literally 1, but works for this
				 * program */
				newclause = 1;
			}
			continue;
		default:
			break;
		}
		/* Either inside comment or Nth word of clause. */
		if (comment == 1 || (newclause == 0 && argument == ITS_NOTHING_INTERESTING))
			continue;
		/* Strip white spaces before new clause word. */
		if ((newclause == 1 || argument != ITS_NOTHING_INTERESTING)
		    && isspace(c) && i == 0 && one_ip_range == 0)
			continue;
		/* Save to word which clause this is. */
		if ((newclause == 1 || argument != ITS_NOTHING_INTERESTING)
		    && (!isspace(c) || 0 < quote)) {
			word[i] = c;
			i++;
			/* Long word which is almost causing overflow. None
			 * of words are this long which the program is
			 * searching. */
			if (MAXLEN == i) {
				newclause = 0;
				i = 0;
				continue;
			}
		}
		/* See if clause is something that parser is looking for. */
		else if (newclause == 1) {
			/* Insert string end & set state */
			word[i] = '\0';
			if (word[i - 1] != '{')
				newclause = 0;
			i = 0;
			argument = is_interesting_config_clause(word);
			if (argument == ITS_A_RANGE_FIRST_IP)
				one_ip_range = 1;
		}
		/* words after range, shared-network or include */
		else if (argument != ITS_NOTHING_INTERESTING) {
			word[i] = '\0';
			newclause = 0;
			i = 0;

			switch (argument) {
			case ITS_A_RANGE_SECOND_IP:
				/* printf ("range 2nd ip: %s\n", word); */
				range_p = ranges + num_ranges;
				argument = ITS_NOTHING_INTERESTING;
				parse_ipaddr(word, &addr);
				if (one_ip_range == 1) {
					one_ip_range = 0;
					copy_ipaddr(&range_p->first_ip, &addr);
				}
				copy_ipaddr(&range_p->last_ip, &addr);
				reorder_last_first(range_p);
 newrange:
				range_p->count = 0;
				range_p->touched = 0;
				range_p->backups = 0;
				range_p->shared_net = shared_p;
				num_ranges++;
				if (RANGES < num_ranges + 1) {
					RANGES *= 2;
					ranges = xrealloc(ranges, sizeof(struct range_t) * RANGES);
					range_p = ranges + num_ranges;
				}
				newclause = 1;
				break;
			case ITS_A_RANGE_FIRST_IP:
				/* printf ("range 1nd ip: %s\n", word); */
				range_p = ranges + num_ranges;
				if (!(parse_ipaddr(word, &addr)))
					/* word was not ip, try again */
					break;
				copy_ipaddr(&range_p->first_ip, &addr);
				one_ip_range = 0;
				argument = ITS_A_RANGE_SECOND_IP;
				break;
			case ITS_A_SHAREDNET:
				/* printf ("shared-network named: %s\n", word); */
				num_shared_networks++;
				shared_p = shared_networks + num_shared_networks;
				shared_p->name = xstrdup(word);
				shared_p->available = 0;
				shared_p->used = 0;
				shared_p->touched = 0;
				shared_p->backups = 0;
				if (SHARED_NETWORKS < num_shared_networks + 2)
					/* FIXME: make this to go away by reallocating more space. */
					error(EXIT_FAILURE, 0,
					      "parse_config: increase default.h SHARED_NETWORKS and recompile");
				argument = ITS_NOTHING_INTERESTING;
				braces_shared = braces;
				break;
			case ITS_AN_INCLUCE:
				/* printf ("include file: %s\n", word); */
				argument = ITS_NOTHING_INTERESTING;
				parse_config(0, word, shared_p);
				newclause = 1;
				break;
			case ITS_NOTHING_INTERESTING:
				/* printf ("nothing interesting: %s\n", word); */
				argument = ITS_NOTHING_INTERESTING;
				break;
			default:
				puts("impossible occurred, report a bug");
				abort();
			}
		}
	}
	free(word);
	fclose(dhcpd_config);
	return;
}

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

#include <stdlib.h>
#include <stdio.h>

#include "dhcpd-pools.h"

/* Clean up data */
int prepare_data(void)
{
	unsigned long int i, j, k;

	/* Sort leases */
	qsort(leases, (size_t) num_leases, sizeof(long int), &intcomp);

	/* Get rid of lease dublicates */
	for (k = j = i = 0; i < num_leases; i++) {
		if (j != leases[i]) {
			leases[k] = leases[i];
			j = leases[i];
			k++;
		}
	}
	num_leases = k;

	/* Delete touched IPs that are in use. */
	j = 0;
	for (i = 0; i < num_touches; i++) {
		if (bsearch
		    (&touches[i], leases, (size_t) num_leases,
		     sizeof(long int), &intcomp) == NULL) {
			touches[j] = touches[i];
			j++;
		}
	}
	num_touches = j;
	qsort(touches, (size_t) num_touches, sizeof(long int), &intcomp);

	/* Sort ranges */
	qsort(ranges, (size_t) num_ranges, sizeof(struct range_t),
	      &rangecomp);

	/* Sort backups */
	if (0 < num_backups) {
		qsort(backups, (size_t) num_backups, sizeof(long int),
		      &intcomp);
	}

	return 0;
}

/* Join leases and ranges into couter structs */
int do_counting(void)
{
	struct range_t *range_p;
	unsigned int i, j, k, m, block_size;

	i = j = m = 0;
	range_p = ranges;

	/* Walk through ranges */
	for (k = 0; k < num_ranges; k++) {
		/* Count IPs in use */
		for (; leases[i] < range_p->last_ip
		     && (unsigned long) i < num_leases; i++) {
			if (leases[i] < range_p->first_ip) {
				continue;
			}
			/* IP with in range */
			range_p->count++;
			if (range_p->shared_net) {
				range_p->shared_net->used++;
			}
		}

		/* Count touched IPs */
		for (; touches[j] < range_p->last_ip
		     && (unsigned long) j < num_touches; j++) {
			if (touches[j] < range_p->first_ip) {
				continue;
			}
			/* IP with in range */
			range_p->touched++;
			if (range_p->shared_net) {
				range_p->shared_net->touched++;
			}
		}

		/* Count backup IPs */
		if (0 < num_backups) {
			for (; backups[m] < range_p->last_ip
			     && (unsigned long) m < num_touches; m++) {
				if (touches[m] < range_p->first_ip) {
					continue;
				}
				/* IP with in range */
				range_p->backups++;
				if (range_p->shared_net) {
					range_p->shared_net->backups++;
				}
			}
		}

		/* Size of range, shared net & all networks */
		block_size =
		    (unsigned int) (range_p->last_ip - range_p->first_ip -
				    1);
		if (range_p->shared_net) {
			range_p->shared_net->available += block_size;
		}

		/* Reverse so that not even a one IP will be missed. */
		if (i) {
			i--;
		}
		if (j) {
			j--;
		}

		range_p++;
	}

	/* During count of other shared networks default network and
	 * all networks got mixed to gether semantically. This fixes
	 * the problem, but is not elegant. TODO: fix semantics of all
	 * and default share_network. */
	shared_networks->available = 0;
	shared_networks->used = 0;
	shared_networks->touched = 0;
	range_p = ranges;
	for (k = 0; k < num_ranges; k++) {
		shared_networks->available +=
		    range_p->last_ip - range_p->first_ip - 1;
		shared_networks->used += range_p->count;
		shared_networks->touched += range_p->touched;
		shared_networks->backups += range_p->backups;
		range_p++;
	}

	return 0;
}

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

/* #include <config.h> */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "dhcpd-pools.h"

/* Clean up data */
int prepare_data(void)
{
	unsigned long int i, j, k;

	/* Sort leases */
	qsort(leases, (size_t) num_leases, sizeof(uint32_t), &intcomp);

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
	qsort(touches, (size_t) num_touches, sizeof(uint32_t), &intcomp);

	/* Sort ranges */
	qsort(ranges, (size_t) num_ranges, sizeof(struct range_t),
	      &rangecomp);

	/* Sort backups */
	if (0 < num_backups) {
		qsort(backups, (size_t) num_backups, sizeof(uint32_t),
		      &intcomp);
	}

	return 0;
}

/* Join leases and ranges into couter structs */
int do_counting(void)
{
	struct range_t *range_p;
	unsigned int i, j, k, l, block_size;

	range_p = ranges;

	/* Walk through ranges */
	for (i = j = k = l = 0; i < num_ranges; i++) {
		/* Count IPs in use */
		for (; leases[j] < range_p->last_ip
		     && (unsigned long) j < num_leases; j++) {
			if (leases[j] < range_p->first_ip) {
				continue;
			}
			/* IP with in range */
			range_p->count++;
			if (range_p->shared_net) {
				range_p->shared_net->used++;
			}
		}

		/* Count touched IPs */
		for (; touches[k] < range_p->last_ip
		     && (unsigned long) k < num_touches; k++) {
			if (touches[k] < range_p->first_ip) {
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
			for (; backups[l] < range_p->last_ip
			     && (unsigned long) l < num_touches; l++) {
				if (touches[l] < range_p->first_ip) {
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

		/* Go backwards one step so that not even a one IP will be
		 * missed. This is possibly always unnecessary. */
		if (j) {
			j--;
		}
		if (k) {
			k--;
		}

		range_p++;
	}

	/* FIXME: During count of other shared networks default network and
	 * all networks got mixed to gether semantically. This fixes the
	 * problem, but is not elegant. */
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

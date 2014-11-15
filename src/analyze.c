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

/*! \file analyze.c
 * \brief Data analysis functions.
 */

#include <config.h>

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "dhcpd-pools.h"

/*! \brief Prepare data for analysis. The function will sort leases and
 * ranges.
 * FIXME: This function should return void. */
int prepare_data(void)
{
	/* Sort leases */
	HASH_SORT(leases, leasecomp);
	/* Sort ranges */
	qsort(ranges, (size_t)num_ranges, sizeof(struct range_t), &rangecomp);
	return 0;
}

/*! \brief Perform counting.  Join leases with ranges, and update counters.
 * FIXME: This function should return void. */
int do_counting(void)
{
	struct range_t *restrict range_p;
	const struct leases_t *restrict l = leases;
	unsigned long i, k, block_size;

	/* Walk through ranges */
	range_p = ranges;
	for (i = 0; i < num_ranges; i++) {
		for (; l != NULL && ipcomp(&range_p->first_ip, &l->ip) < 0; l = l->hh.prev)
			/* rewind */ ;
		if (l == NULL)
			l = leases;
		for (; l != NULL && ipcomp(&l->ip, &range_p->last_ip) <= 0; l = l->hh.next) {
			if (ipcomp(&l->ip, &range_p->first_ip) < 0)
				continue;	/* cannot happen? */
			/* IP in range */
			switch (l->type) {
			case FREE:
				range_p->touched++;
				break;
			case ACTIVE:
				range_p->count++;
				break;
			case BACKUP:
				range_p->backups++;
				break;
			}
			if (range_p->shared_net) {
				switch (l->type) {
				case FREE:
					range_p->shared_net->touched++;
					break;
				case ACTIVE:
					range_p->shared_net->used++;
					break;
				case BACKUP:
					range_p->shared_net->backups++;
					break;
				}
			}
		}
		/* Size of range, shared net & all networks */
		block_size = get_range_size(range_p);
		if (range_p->shared_net)
			range_p->shared_net->available += block_size;
		range_p++;
	}
	/* FIXME: During count of other shared networks default network
	 * and all networks got mixed together semantically.  The below
	 * fixes the problem, but is not elegant.  */
	shared_networks->available = 0;
	shared_networks->used = 0;
	shared_networks->touched = 0;
	range_p = ranges;
	for (k = 0; k < num_ranges; k++) {
		shared_networks->available += get_range_size(range_p);
		shared_networks->used += range_p->count;
		shared_networks->touched += range_p->touched;
		shared_networks->backups += range_p->backups;
		range_p++;
	}
	return 0;
}

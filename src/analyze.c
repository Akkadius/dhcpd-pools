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

/* #include <config.h> */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "dhcpd-pools.h"

/* Clean up data */
int ip_sort(struct leases_t *a, struct leases_t *b)
{
	return (a->ip - b->ip);
}

int prepare_data(void)
{
	/* Sort leases */
	HASH_SORT(leases, ip_sort);
	return 0;
}

/* Join leases and ranges into couter structs */
int do_counting(void)
{
	struct range_t *range_p;
	struct leases_t *l;
	unsigned long i, k, block_size;

	range_p = ranges;

	/* Walk through ranges */
	for (i = 0; i < num_ranges; i++) {
		/* Count IPs in use */
		for (l = leases; l != NULL && range_p->last_ip >= l->ip; l = l->hh.next) {
			if (l->ip < range_p->first_ip) {
				/* should not be necessary */
				continue;
			}
			/* IP in range */
			switch (l->type) {
			case ACTIVE:
				range_p->count++;
				break;
			case FREE:
				range_p->touched++;
				break;
			case BACKUP:
				range_p->backups++;
				break;
			}

			if (range_p->shared_net) {
				switch (l->type) {
				case ACTIVE:
					range_p->shared_net->used++;
					break;
				case FREE:
					range_p->shared_net->touched++;
					break;
				case BACKUP:
					range_p->shared_net->backups++;
					break;
				}
			}
		}

		/* Size of range, shared net & all networks */
		block_size =
		    (unsigned int)(range_p->last_ip - range_p->first_ip - 1);
		if (range_p->shared_net) {
			range_p->shared_net->available += block_size;
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

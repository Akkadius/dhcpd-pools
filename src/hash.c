/*
 * The dhcpd-pools has BSD 2-clause license which also known as "Simplified
 * BSD License" or "FreeBSD License".
 *
 * Copyright 2012- Enno Gröper. All rights reserved.
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

#include "dhcpd-pools.h"

void add_lease(int ip, enum ltype type)
{
	struct leases_t *l;
	l = safe_malloc(sizeof(struct leases_t));
	l->ip = ip;
	l->type = type;
	HASH_ADD_INT(leases, ip, l);
}

struct leases_t *find_lease(int ip)
{
	struct leases_t *l;

	HASH_FIND_INT(leases, &ip, l);
	return l;
}

void delete_lease(struct leases_t *lease)
{
	HASH_DEL(leases, lease);
	free(lease);
}

/* uthash >= 1.9.2
void delete_all_leases()
{
	struct leases_t *l, *tmp;
	HASH_ITER(hh, leases, l, tmp) {
		HASH_DEL(leases, l);
		free(l);
	}
}
*/

void delete_all_leases()
{
	struct leases_t *l;
	while (leases) {
		l = leases;
		HASH_DEL(leases, l);	/* leases advances to next on delete */
		free(l);
	}
}

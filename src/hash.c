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

/*! \file hash.c
 * \brief The leases hash functions. The hash sorting is key to make
 * analysis happen as quick as possible..
 */

#include "dhcpd-pools.h"
#include "xalloc.h"

#define HASH_FIND_V6(head, findv6, out) HASH_FIND(hh, head, findv6, 16, out)
#define HASH_ADD_V6(head, v6field, add) HASH_ADD(hh, head, v6field, 16, add)

/*! \brief Add a lease to hash array.
 * \param addr Binary IP to be added in leases hash.
 * \param type Lease state of the IP. */
void add_lease_init(union ipaddr_t *addr __attribute__((unused)), enum ltype type __attribute__((unused)))
{
}

void add_lease_v4(union ipaddr_t *addr, enum ltype type)
{
	struct leases_t *l;
	l = xmalloc(sizeof(struct leases_t));
	copy_ipaddr(&l->ip, addr);
	l->type = type;
	HASH_ADD_INT(leases, ip.v4, l);
	l->ethernet = NULL;
}

void add_lease_v6(union ipaddr_t *addr, enum ltype type)
{
	struct leases_t *l;
	l = xmalloc(sizeof(struct leases_t));
	copy_ipaddr(&l->ip, addr);
	l->type = type;
	HASH_ADD_V6(leases, ip.v6, l);
	l->ethernet = NULL;
}

/*! \brief Find pointer to lease from hash array.
 * \param addr Binary IP searched from leases hash.
 * \return A lease structure about requested IP, or NULL.
 */
struct leases_t *find_lease_init(union ipaddr_t *addr __attribute__((unused)))
{
	return NULL;
}

struct leases_t *find_lease_v4(union ipaddr_t *addr)
{
	struct leases_t *l;
	HASH_FIND_INT(leases, &addr->v4, l);
	return l;
}

struct leases_t *find_lease_v6(union ipaddr_t *addr)
{
	struct leases_t *l;
	HASH_FIND_V6(leases, &addr->v4, l);
	return l;
}

/*! \brief Delete a lease from hash array.
 * \param lease Pointer to lease hash. */
void delete_lease(struct leases_t *lease)
{
	free(lease->ethernet);
	HASH_DEL(leases, lease);
	free(lease);
}

/*! \brief Delete all leases from hash array. */
#ifdef HASH_ITER
void delete_all_leases(void)
{
	struct leases_t *l, *tmp;
	HASH_ITER(hh, leases, l, tmp) {
		free(l->ethernet);
		HASH_DEL(leases, l);
		free(l);
	}
}
#else
void delete_all_leases(void)
{
	while (leases) {
		struct leases_t *l;
		l = leases;
		free(l->ethernet);
		HASH_DEL(leases, l);	/* leases advances to next on delete */
		free(l);
	}
}
#endif

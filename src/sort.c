/*
 * The dhcpd-pools has BSD 2-clause license which also known as "Simplified
 * BSD License" or "FreeBSD License".
 *
 * Copyright 2006- Sami Kerola. All rights reserved.
 * Copyright 2012 Cheer Xiao.
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

/*! \file sort.c
 * \brief Functions to sort output.
 */

#include <config.h>

#include <err.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "dhcpd-pools.h"

/*! \brief Compare IP address, with IPv4/v6 determination.
 * \param a Binary IP address.
 * \param b Binary IP address.
 * \return If a < b return -1, if a < b return 1, when they are equal return 0.
 */
int ipcomp(const union ipaddr_t *restrict a, const union ipaddr_t *restrict b)
{
	if (config.dhcp_version == VERSION_6) {
		return memcmp(&a->v6, &b->v6, sizeof(a->v6));
	} else {
		if (a->v4 < b->v4)
			return -1;
		if (a->v4 > b->v4)
			return 1;
		return 0;
	}
}

/*! \brief Compare IP address in leases. Suitable for sorting leases.
 * \param a A lease structure.
 * \param b A lease structure.
 * \return Return pas through from ipcomp.
 */
int leasecomp(const void *restrict a, const void *restrict b)
{
	return ipcomp(&((const struct leases_t *)a)->ip,
		      &((const struct leases_t *)b)->ip);
}

/*! \brief Compare IP address in leases. Suitable for sorting range table.
 * \param r1 A range structure.
 * \param r2 A range structure.
 * \return Return pas through from ipcomp.
 */
int rangecomp(const void *restrict r1, const void *restrict r2)
{
	return ipcomp(&((const struct range_t *)r1)->first_ip,
		      &((const struct range_t *)r2)->first_ip);
}

/*! \brief Compare two unsigned long.
 * \param u1,u2 Data to compare.
 * \return Like strcmp.
 */
int comp_ulong(unsigned long u1, unsigned long u2)
{
	return u1 < u2 ? -1 : u1 > u2 ? 1 : 0;
}

/*! \brief Compare two range_t by their first_ip.
 * \param r1,r2 Pointers to data to compare.
 * \return Like strcmp.
 */
int comp_ip(struct range_t *r1, struct range_t *r2)
{
	return ipcomp(&r1->first_ip, &r2->first_ip);
}

/*! \brief Compare two range_t by their capacity.
 * \param r1,r2 Pointers to data to compare.
 * \return Like strcmp.
 */
int comp_max(struct range_t *r1, struct range_t *r2)
{
	return comp_ulong(get_range_size(r1), get_range_size(r2));
}

/*! \brief Compare two range_t by their current usage.
 * \param r1,r2 Pointers to data to compare.
 * \return Like strcmp.
 */
int comp_cur(struct range_t *r1, struct range_t *r2)
{
	return comp_ulong(r1->count, r2->count);
}

/*! \brief Compare two range_t by their current usage percentage.
 * \param r1,r2 Pointers to data to compare.
 * \return Like strcmp.
 */
int comp_percent(struct range_t *r1, struct range_t *r2)
{
	return comp_ulong(ret_percent(*r1), ret_percent(*r2));
}

/*! \brief Compare two range_t by their touched addresses.
 * \param r1,r2 Pointers to data to compare.
 * \return Like strcmp.
 */
int comp_touched(struct range_t *r1, struct range_t *r2)
{
	return comp_ulong(r1->touched, r2->touched);
}

/*! \brief Compare two range_t by their touched and in use addresses.
 * \param r1,r2 Pointers to data to compare.
 * \return Like strcmp.
 */
int comp_tc(struct range_t *r1, struct range_t *r2)
{
	return comp_ulong(ret_tc(*r1), ret_tc(*r2));
}

/*! \brief Compare two range_t by their touched and in use percentage.
 * \param r1,r2 Pointers to data to compare.
 * \return Like strcmp.
 */
int comp_tcperc(struct range_t *r1, struct range_t *r2)
{
	return comp_ulong(ret_tcperc(*r1), ret_tcperc(*r2));
}

/*! \brief Percentage in use in range.
 * \param r A range structure.
 * \return Usage percentage of the given range.
 */
unsigned long int ret_percent(struct range_t r)
{
	float f;
	f = (float)r.count / get_range_size(&r);
	return ((unsigned long int)(f * 100000));
}

/*! \brief Touched and in use in range
 * \param r A range structure.
 * \return Number of touched or in use addresses in the given range.
 */
unsigned long int ret_tc(struct range_t r)
{
	return (r.count + r.touched);
}

/*! \brief Return percentage of addresses touched and in use in range.
 * \param r A range structure.
 * \return Percentage of touched or in use addresses in the given range.
 */
unsigned long int ret_tcperc(struct range_t r)
{
	float f;
	f = (float)(r.count + r.touched) / get_range_size(&r);
	return ((unsigned long int)(f * 10000));
}

/*! \brief Sort field selector.
 * \param c Symbolic name of a sort by character.
 * The sort algorithms are stabile, which means multiple sorts can be
 * specified and they do not mess the result of previous sort.  The sort
 * algorithms are used via function pointer, that gets to be reassigned.
 */
void field_selector(char c)
{
	switch (c) {
	case 'n':
		break;
	case 'i':
		comparer = comp_ip;
		break;
	case 'm':
		comparer = comp_max;
		break;
	case 'c':
		comparer = comp_cur;
		break;
	case 'p':
		comparer = comp_percent;
		break;
	case 't':
		comparer = comp_touched;
		break;
	case 'T':
		comparer = comp_tc;
		break;
	case 'e':
		comparer = comp_tcperc;
		break;
	default:
		warnx("field_selector: unknown sort order `%c'", c);
		errx(EXIT_FAILURE, "Try `%s --help' for more information.",
		     program_invocation_short_name);
	}
}

/*! \brief Perform requested sorting.
 * \param left The left side of the merge sort.
 * \param right The right side of the merge sort.
 * \return Relevant for merge sort decision.
 */
static int merge(struct range_t *restrict left, struct range_t *restrict right)
{
	int i, len, ret;
	int cmp;

	len = strlen(config.sort);
	for (i = 0; i < len; i++) {
		/* Handling strings is case of it's own */
		if (config.sort[i] == 'n') {
			ret =
			    strcmp(left->shared_net->name,
				   right->shared_net->name);
			if (0 < ret)
				return (0);
			if (ret < 0)
				return (1);
			continue;
		}

		/* Select which function is pointed by comparer */
		field_selector(config.sort[i]);
		cmp = comparer(left, right);
		/* If fields are equal use next sort method */
		if (cmp == 0) {
			continue;
		}
		if (cmp < 0) {
			return (1);
		}
		return (0);
	}

	/* If all comparers where equal */
	return (0);
}

/*! \brief Mergesort for range table.
 * \param orig Pointer to range that is requested to be sorted.
 * \param size Number of ranges to be sorted.
 * \param temp Temporary memory space, needed when a values has to be
 * flipped.
 */
void mergesort_ranges(struct range_t *restrict orig, int size,
		      struct range_t *restrict temp)
{
	int left, right, i;
	struct range_t hold;
	/* Merge sort split size */
	static const int MIN_MERGE_SIZE = 8;

	if (size < MIN_MERGE_SIZE) {
		for (left = 0; left < size; left++) {
			hold = *(orig + left);
			for (right = left - 1; 0 <= right; right--) {
				if (merge((orig + right), &hold)) {
					break;
				}
				*(orig + right + 1) = *(orig + right);
			}
			*(orig + right + 1) = hold;
		}
		return;
	}

	mergesort_ranges(orig, size / 2, temp);
	mergesort_ranges(orig + size / 2, size - size / 2, temp);

	left = 0;
	right = size / 2;
	i = 0;

	while (left < size / 2 && right < size) {
		if (merge((orig + left), (orig + right))) {
			*(temp + i) = *(orig + left);
			left++;
		} else {
			*(temp + i) = *(orig + right);
			right++;
		}
		i++;
	}
	while (left < size / 2) {
		*(temp + i) = *(orig + left);
		left++;
		i++;
	}
	while (right < size) {
		*(temp + i) = *(orig + right);
		right++;
		i++;
	}

	memcpy(orig, temp, size * sizeof(struct range_t));
}

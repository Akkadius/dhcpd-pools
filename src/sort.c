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

#include <config.h>

#include <err.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "dhcpd-pools.h"

/* Sort functions for range sorting */
int intcomp(const void *x, const void *y)
{
	if (*(uint32_t *) x < *(uint32_t *) y)
		return -1;
	else if (*(uint32_t *) y < *(uint32_t *) x)
		return 1;
	else
		return 0;
}

int rangecomp(const void *r1, const void *r2)
{
	if ((((struct range_t *)r1)->first_ip) <
	    (((struct range_t *)r2)->first_ip))
		return -1;
	else if ((((struct range_t *)r2)->first_ip) <
		 (((struct range_t *)r1)->first_ip))
		return 1;
	else
		return 0;
}

unsigned long int ret_ip(struct range_t r)
{
	return (r.first_ip);
}

unsigned long int ret_cur(struct range_t r)
{
	return (r.count);
}

unsigned long int ret_max(struct range_t r)
{
	return (r.last_ip - r.first_ip);
}

unsigned long int ret_percent(struct range_t r)
{
	float f;
	f = (float)r.count / (r.last_ip - r.first_ip - 1);
	return ((unsigned long int)(f * 100000));
}

unsigned long int ret_touched(struct range_t r)
{
	return (r.touched);
}

unsigned long int ret_tc(struct range_t r)
{
	return (r.count + r.touched);
}

unsigned long int ret_tcperc(struct range_t r)
{
	float f;
	f = (float)(r.count + r.touched) / (r.last_ip - r.first_ip - 1);
	return ((unsigned long int)(f * 10000));
}

void field_selector(char c)
{
	switch (c) {
	case 'n':
		break;
	case 'i':
		returner = ret_ip;
		break;
	case 'm':
		returner = ret_max;
		break;
	case 'c':
		returner = ret_cur;
		break;
	case 'p':
		returner = ret_percent;
		break;
	case 't':
		returner = ret_touched;
		break;
	case 'T':
		returner = ret_tc;
		break;
	case 'e':
		returner = ret_tcperc;
		break;
	default:
		warnx("field_selector: unknown sort order `%c'", c);
		errx(EXIT_FAILURE, "Try `%s --help' for more information.",
		     program_invocation_short_name);
	}
}

/* Needed to support multiple key sorting. */
int get_order(struct range_t *left, struct range_t *right)
{
	int i, len, ret;
	unsigned long int lint, rint;

	len = strlen(config.sort);
	for (i = 0; i < len; i++) {
		/* Handling strings is case of it's own */
		if (config.sort[i] == 'n') {
			ret =
			    strcmp(left->shared_net->name,
				   right->shared_net->name);
			if (0 < ret) {
				return (0);
			} else if (ret < 0) {
				return (1);
			} else {
				continue;
			}
		}

		/* Select which function is pointed by returner */
		field_selector(config.sort[i]);
		lint = returner(*left);
		rint = returner(*right);
		/* If fields are equal use next sort method */
		if (lint == rint) {
			continue;
		}
		if (lint < rint) {
			return (1);
		}
		return (0);
	}

	/* If all returners where equal */
	return (0);
}

void mergesort_ranges(struct range_t *orig, int size, struct range_t *temp)
{
	int left, right, i;
	struct range_t hold;
	/* Merge sort split size */
	static const int MIN_MERGE_SIZE = 8;

	if (size < MIN_MERGE_SIZE) {
		for (left = 0; left < size; left++) {
			hold = *(orig + left);
			for (right = left - 1; 0 <= right; right--) {
				if (get_order((orig + right), &hold)) {
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
		if (get_order((orig + left), (orig + right))) {
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

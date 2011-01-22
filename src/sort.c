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
#include <string.h>
#include <err.h>

#include "dhcpd-pools.h"
#include "defaults.h"

/* Sort functions for range sorting */
int intcomp(const void *x, const void *y)
{
	if (*(unsigned long int *) x < *(unsigned long int *) y)
		return -1;
	else if (*(unsigned long int *) y < *(unsigned long int *) x)
		return 1;
	else
		return 0;
}

int rangecomp(const void *r1, const void *r2)
{
	if ((((struct range_t *) r1)->first_ip) <
	    (((struct range_t *) r2)->first_ip))
		return -1;
	else if ((((struct range_t *) r2)->first_ip) <
		 (((struct range_t *) r1)->first_ip))
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
	f = (float) r.count / (r.last_ip - r.first_ip - 1);
	return ((unsigned long int) (f * 100000));
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
	f = (float) (r.count + r.touched) / (r.last_ip - r.first_ip - 1);
	return ((unsigned long int) (f * 10000));
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
		errx(EXIT_FAILURE,
		     "field_selector: unknown sort order `%c'",
		     config.sort[0]);
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

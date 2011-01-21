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

#ifndef DHCPD_POOLS_H
# define DHCPD_POOLS_H 1

#include <config.h>

/* Feature test switches */
#define _POSIX_SOURCE 1
#define POSIXLY_CORRECT 1

#ifdef  HAVE_STDLIB_H
#include <stdlib.h>
#else /* Not STDC_HEADERS */
extern void exit ();
extern char *malloc ();
#endif /* STDC_HEADERS */

/* Structures and unions */
struct configuration_t
{
  char *dhcpdconf_file;
  char *dhcpdlease_file;
  char output_format[2];
  char sort[6];
  int reverse_order;
  char *output_file;
  int output_limit[2];
};
struct shared_network_t
{
  char *name;
  unsigned long int available;
  unsigned long int used;
  unsigned long int touched;
  unsigned long int backups;
};
struct range_t
{
  struct shared_network_t *shared_net;
  unsigned long int first_ip;
  unsigned long int last_ip;
  unsigned long int count;
  unsigned long int touched;
  unsigned long int backups;
};
/* Global variables */
static int true = 1;
static int false = 0;

char *program_name;
struct configuration_t config;

static int output_limit_bit_1 = 1;
static int output_limit_bit_2 = 2;
static int output_limit_bit_3 = 4;
unsigned int fullhtml;

struct shared_network_t *shared_networks;
char *shared_net_names;
unsigned int num_shared_networks;

struct range_t *ranges;
unsigned int num_ranges;

unsigned long int *leases;
unsigned long int num_leases;

unsigned long int *touches;
unsigned long int num_touches;

unsigned long int *backups;
unsigned long int num_backups;

/* Function prototypes */
int prepare_memory (void);
int parse_leases (void);
char * parse_config (int, char *, char *, char *, struct shared_network_t *);
int nth_field (int n, char *dest, const char *src);
int prepare_data (void);
int do_counting (void);
void flip_ranges(struct range_t *ranges, struct range_t *tmp_ranges);
/* General support functions */
void *safe_malloc (size_t size);
void eprintf (char *, ...);
void print_version (void);
void usage (int status);
/* qsort required functions... */
/* ...for ranges and... */
int intcomp (const void *x, const void *y);
int rangecomp (const void *r1, const void *r2);
/* sort function pointer and functions */
int sort_name (void);
unsigned long int (*returner) (struct range_t r);
unsigned long int ret_ip(struct range_t r);
unsigned long int ret_cur(struct range_t r);
unsigned long int ret_max(struct range_t r);
unsigned long int ret_percent(struct range_t r);
unsigned long int ret_touched(struct range_t r);
unsigned long int ret_tc(struct range_t r);
unsigned long int ret_tcperc(struct range_t r);
void field_selector(char c);
int get_order(struct range_t *left, struct range_t *right);
void mergesort_ranges (struct range_t *orig, int size, struct range_t *temp);
/* output function pointer and functions */
int (*output_analysis) (void);
int output_txt (void);
int output_html (void);
int output_xml (void);
int output_csv (void);
/* Memory release, file closing etc */
void clean_up (void);

#endif /* DHCPD_POOLS_H */

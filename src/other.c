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

#include <config.h>

#include "dhcpd-pools.h"

#ifdef  HAVE_STDLIB_H
#include <stdlib.h>
#else
extern void exit();
extern char *malloc();
extern void _exit();
#endif

#ifdef  HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include <err.h>
#include <errno.h>
#ifdef HAVE_ERROR_H
#include <error.h>
#endif
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#ifdef HAVE_STDIO_EXT_H
#include <stdio_ext.h>
#endif
#include <unistd.h>

#ifndef HAVE_ERROR
#ifdef __FreeBSD__
#define error errc
#endif
#endif

#ifndef HAVE___FPENDING
static size_t __fpending(FILE *fp)
{
	return (fp->_p - fp->_bf._base);
}
#endif

/* Simple memory allocation wrapper */
void *safe_malloc(const size_t size)
{
	void *ret = malloc(size);
	if (ret == NULL) {
		err(EXIT_FAILURE,
		    "safe_malloc: cannot allocate %lu bytes: ", size);
	}

	return ret;
}

/* Simple memory reallocation wrapper */
void *safe_realloc(void *ptr, const size_t size)
{
	void *ret = realloc(ptr, size);

	if (!ret && size)
		err(EXIT_FAILURE,
		    "safe_realloc: cannot allocate %lu bytes", size);
	return ret;
}

int
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
    __attribute__ ((hot))
#endif
    xstrstr(char *restrict a, const char *restrict b, int len)
{
	int i;
	/* two spaces are very common in lease file, after them
	 * nearly everything differs */
	if (a[2] != b[2]) {
		return false;
	}
	/* "  binding state " == 16 chars, this will skip right
	 * to first difering line. */
	if (17 < len && a[17] != b[17]) {
		return false;
	}
	/* looking good, double check the whole thing... */
	for (i = 0; a[i] != '\0' && b[i] != '\0'; i++) {
		if (a[i] != b[i]) {
			return false;
		}
	}
	return true;
}

/* Simple strdup wrapper */
char *safe_strdup(const char *restrict str)
{
	char *ret = strdup(str);

	if (!ret && str)
		err(EXIT_FAILURE, "cannot duplicate string");
	return ret;
}

/* Return percentage value */
double strtod_or_err(const char *restrict str, const char *restrict errmesg)
{
	double num;
	char *end = NULL;

	if (str == NULL || *str == '\0')
		goto err;
	errno = 0;
	num = strtod(str, &end);

	if (errno || str == end || (end && *end))
		goto err;

	return num;
 err:
	if (errno)
		err(EXIT_FAILURE, "%s: '%s'", errmesg, str);

	errx(EXIT_FAILURE, "%s: '%s'", errmesg, str);
}

void flip_ranges(struct range_t *restrict flip_me,
		 struct range_t *restrict tmp_ranges)
{
	unsigned int i = num_ranges - 1, j;

	for (j = 0; j < num_ranges; j++) {
		*(tmp_ranges + j) = *(flip_me + i);
		i--;
	}

	memcpy(flip_me, tmp_ranges, num_ranges * sizeof(struct range_t));
}

/* Free memory, flush buffers etc */
void clean_up(void)
{
	unsigned int i;

	/* Just in case there something in buffers */
	if (fflush(NULL)) {
		warn("clean_up: fflush");
	}
	num_shared_networks++;
	for (i = 0; i < num_shared_networks; i++) {
		free((shared_networks + i)->name);
	}
	free(config.dhcpdconf_file);
	free(config.dhcpdlease_file);
	free(config.output_file);
	free(ranges);
	delete_all_leases();
	free(shared_networks);
}

int close_stream(FILE *stream)
{
	const int some_pending = (__fpending(stream) != 0);
	const int prev_fail = (ferror(stream) != 0);
	const int fclose_fail = (fclose(stream) != 0);
	if (prev_fail || (fclose_fail && (some_pending || errno != EBADF))) {
		if (!fclose_fail)
			errno = 0;
		return EOF;
	}
	return 0;
}

/* Use atexit(); */
void close_stdout(void)
{
	if (close_stream(stdout) != 0 && !(errno == EPIPE)) {
		char const *write_error = "write error";
		error(0, errno, "%s", write_error);
		_exit(EXIT_FAILURE);
	}

	if (close_stream(stderr) != 0)
		_exit(EXIT_FAILURE);
}

void __attribute__ ((__noreturn__)) print_version(void)
{
	fprintf(stdout, "%s\n"
		"Written by Sami Kerola.\n"
		"XML support by Dominic Germain, Sogetel inc.\n\n"
		"The software has FreeBSD License.\n", PACKAGE_STRING);
	exit(EXIT_SUCCESS);
}

void __attribute__ ((__noreturn__)) usage(int status)
{
	FILE *out;
	out = status != 0 ? stderr : stdout;

	fprintf(out, "\
Usage: %s [OPTIONS]\n\n\
This is ISC dhcpd pools usage analyzer.\n\
\n", program_invocation_short_name);
	fprintf(out, "\
  -c, --config=FILE      path to the dhcpd.conf file\n\
  -l, --leases=FILE      path to the dhcpd.leases file\n\
  -f, --format=[thHcxX]  output format\n\
                           t for text\n\
                           h for html table\n\
                           H for full html page\n\
                           x for xml\n\
                           X for xml with active lease details\n\
                           c for comma separated values\n");
	fprintf(out, "\
  -s, --sort=[nimcptTe]  sort ranges by\n\
                           n name\n\
                           i IP\n\
                           m maxium\n\
                           c current\n\
                           p percent\n\
                           t touched\n\
                           T t+c\n\
                           e t+c perc\n\
  -r, --reverse		 reverse order sort\n\
  -o, --output=FILE      output into a file\n\
  -L, --limit=NR         output limit mask 77 - 00\n");
	fprintf(out, "\
      --warning=PERC     set warning alarming limit\n\
      --critical=PERC    set critical alarming limit\n");
	fprintf(out, "\
  -v, --version          version information\n\
  -h, --help             this screen\n\
\n\
Report bugs to <%s>\n\
Homepage: %s\n", PACKAGE_BUGREPORT, PACKAGE_URL);

	exit(out == stderr ? EXIT_FAILURE : EXIT_SUCCESS);
}

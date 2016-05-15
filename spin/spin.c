/*
 * Copyright (c) 2016 Michael McConville <mmcco@mykolab.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static size_t	nthreads = 4, niters = 1000000, alloc_sz = 64;

void *
thread_func(void *nil)
{
	size_t	 i;
	void	*p;

	for (i = 0; i < niters; i++) {
		if ((p = malloc(alloc_sz)) == NULL)
			err(1, NULL);
		free(p);
	}

	return NULL;
}

int
main(int argc, char *argv[])
{
	struct thread_info	*infos;
	pthread_t		*threads;
	size_t			 i;
	int			 ch, e;
	const char		*errstr;
	void			*p;
	struct timespec		 start, end;
	time_t			 final_time;
	long long		 dur;

	while ((ch = getopt(argc, argv, "t:s:n:")) != -1) {
		switch (ch) {
		case 'n':
			niters = strtonum(optarg, 1, LLONG_MAX, &errstr);
			if (errstr != NULL)
				errx(1, "number of iterations is %s: %s", errstr, optarg);
			break;
		case 's':
			alloc_sz = strtonum(optarg, 1, LLONG_MAX, &errstr);
			if (errstr != NULL)
				errx(1, "allocation size is %s: %s", errstr, optarg);
			break;
		case 't':
			nthreads = strtonum(optarg, 1, LLONG_MAX, &errstr);
			if (errstr != NULL)
				errx(1, "number of threads is %s: %s", errstr, optarg);
			break;
		default:
			errx(1, "%s: [-t nthreads] [-s alloc_size] [-n niters]", getprogname());
		}
	}

	if (niters < nthreads)
		errx(1, "number of iterations must not be less than number of threads");

	threads = reallocarray(NULL, nthreads, sizeof(pthread_t));
	if (threads == NULL)
		err(1, NULL);

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

	for (i = 0; i < nthreads; i++) {
		if ((e = pthread_create(&threads[i], NULL, thread_func, NULL)) != 0)
			errx(1, "pthread_create: %s", strerror(e));
	}

	for (i = 0; i < nthreads; i++) {
		if ((e = pthread_join(threads[i], NULL)) != 0)
			errx(1, "pthread_join: %s", strerror(e));
	}

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);

	dur = (end.tv_sec * 1000000000 + end.tv_nsec) -
	    (start.tv_sec * 1000000000 + start.tv_nsec);

	printf("nthreads = %zu\tniters = %zu\talloc_sz = %zu\tCPU time (nsecs): %lld\n",
			nthreads, niters, alloc_sz, dur);

	return 0;
}

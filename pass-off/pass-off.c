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

#include <err.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct thread_info {
	pthread_cond_t	 cond;
	pthread_mutex_t	 mux;
	void		*p;
};

void *
thread_func(void *p)
{
	struct thread_info	*info = p;

	for (;;) {
		pthread_mutex_lock(&info->mux);
		pthread_cond_wait(&info->cond, &info->mux);
		free(info->p);
		pthread_mutex_unlock(&info->mux);
	}
}

int
main(int argc, char *argv[])
{
	struct thread_info	*infos;
	pthread_t		*threads;
	size_t			 nthreads = 4, niters = 1000000, alloc_sz = 64, i;
	int			 ch, e;
	const char		*errstr;
	struct timespec		 start, end;
	long long		 dur;

	while ((ch = getopt(argc, argv, "n:s:t:")) != -1) {
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
			errx(1, "%s: [-n niters] [-s alloc_size] [-t nthreads]", getprogname());
		}
	}

	infos = reallocarray(NULL, nthreads, sizeof(struct thread_info));
	if (infos == NULL)
		err(1, NULL);

	threads = reallocarray(NULL, nthreads, sizeof(pthread_t));
	if (threads == NULL)
		err(1, NULL);

	for (i = 0; i < nthreads; i++) {
		if ((e = pthread_cond_init(&infos[i].cond, NULL)) != 0)
			errx(1, "pthread_cond_init: %s", strerror(e));

		if ((e = pthread_mutex_init(&infos[i].mux, NULL)) != 0)
			errx(1, "pthread_mutex_init: %s", strerror(e));

		if ((e = pthread_create(&threads[i], NULL, thread_func, &infos[i].cond)) != 0)
			errx(1, "pthread_create: %s", strerror(e));
	}

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

	for (i = 0; i < niters; i++) {
		/* XXX: locking may not be necessary... */
		pthread_mutex_lock(&infos[i % nthreads].mux);

		if ((infos[i % nthreads].p = malloc(alloc_sz)) == NULL)
			err(1, NULL);

		pthread_mutex_unlock(&infos[i % nthreads].mux);

		if ((e = pthread_cond_signal(&infos[i % nthreads].cond)) != 0)
			errx(1, "pthread_cond_signal: %s", strerror(e));
	}

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);

	dur = (end.tv_sec * 1000000000 + end.tv_nsec) -
	    (start.tv_sec * 1000000000 + start.tv_nsec);

	printf("nthreads = %zu\tniters = %zu\talloc_sz = %zu\tCPU time (nsecs): %lld\n",
			nthreads, niters, alloc_sz, dur);

	/*
	 * XXX: avoid destroying because we'd need to signal that we're
	 * done to the worker threads, and that added complexity isn't
	 * worth it here.
	 */
	/*
	for (i = 0; i < nthreads; i++) {
		if ((e = pthread_cond_destroy(&infos[i].cond)) != 0)
			errx(1, "pthread_cond_destroy: %s", strerror(e));
		if ((e = pthread_mutex_destroy(&infos[i].mux)) != 0)
			errx(1, "pthread_mutex_destroy: %s", strerror(e));
	}
	*/

	return 0;
}

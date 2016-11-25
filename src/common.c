/* -*- c-file-style: "linux" -*-
 * Author: Jesper Dangaard Brouer <netoptimizer@brouer.com>, (C)2014
 * License: GPLv2
 * From: https://github.com/netoptimizer/network-testing
 *
 * Common/shared helper functions
 *
 */
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h> /* memset */

#include "global.h"

int verbose = 0;

/* Time code based on:
 *  https://github.com/dterei/Scraps/tree/master/c/time
 *
 * Results
 *  time (sec) => 4ns
 *  ftime (ms) => 39ns
 *  gettimeofday (us) => 30ns
 *  clock_gettime (ns) => 26ns (CLOCK_REALTIME)
 *  clock_gettime (ns) => 8ns (CLOCK_REALTIME_COARSE)
 *  clock_gettime (ns) => 26ns (CLOCK_MONOTONIC)
 *  clock_gettime (ns) => 9ns (CLOCK_MONOTONIC_COARSE)
 *  clock_gettime (ns) => 170ns (CLOCK_PROCESS_CPUTIME_ID)
 *  clock_gettime (ns) => 154ns (CLOCK_THREAD_CPUTIME_ID)
 *  cached_clock (sec) => 0ns
 */

/* gettime returns the current time of day in nanoseconds. */
uint64_t gettime(void)
{
	struct timespec t;
	int res;

	res = clock_gettime(CLOCK_MONOTONIC, &t);
	if (res < 0) {
		fprintf(stderr, "error with gettimeofday! (%i)\n", res);
		exit(EXIT_FAIL_TIME);
	}

	return (uint64_t) t.tv_sec * NANOSEC_PER_SEC + t.tv_nsec;
}

/* Allocate payload buffer */
char *malloc_payload_buffer(int msg_sz)
{
	char * msg_buf = malloc(msg_sz);

	if (!msg_buf) {
		fprintf(stderr, "ERROR: %s() failed in malloc() (caller: 0x%p)\n",
			__func__, __builtin_return_address(0));
		exit(EXIT_FAIL_MEM);
	}
	memset(msg_buf, 0, msg_sz);
	if (verbose)
		fprintf(stderr, " - malloc(msg_buf) = %d bytes\n", msg_sz);
	return msg_buf;
}

/* Fairly general function for timing func call overhead, the function
 * being called/timed is assumed to perform a tight loop, and update
 * the tsc_* and time_* begin and end markers.
 */
int time_func(int loops,
	      int (*func)(int loops, uint64_t* tsc_begin, uint64_t* tsc_end,
			  uint64_t* time_begin, uint64_t* time_end)
	)
{
	uint64_t tsc_begin, tsc_end, tsc_interval;
	uint64_t time_begin, time_end, time_interval;
	double calls_per_sec, ns_per_call, timesec;
	uint64_t tsc_cycles;
	int loops_cnt;

	/*** Loop function being timed ***/
	loops_cnt = func(loops, &tsc_begin, &tsc_end, &time_begin, &time_end);

	tsc_interval  = tsc_end - tsc_begin;
	time_interval = time_end - time_begin;

	if (loops != loops_cnt)
		printf(" WARNING: Loop count(%d) not equal to loops(%d)\n",
		       loops_cnt, loops);

	/* Stats */
	calls_per_sec = loops_cnt / ((double)time_interval / NANOSEC_PER_SEC);
	tsc_cycles    = tsc_interval / loops_cnt;
	ns_per_call   = ((double)time_interval / loops_cnt);
	timesec       = ((double)time_interval / NANOSEC_PER_SEC);

	printf(" Per call: %lu cycles(tsc) %.2f ns\n"
	       "  - %.2f calls per sec (measurement periode time:%.2f sec)\n"
	       "  - (loop count:%d tsc_interval:%lu)\n",
	       tsc_cycles, ns_per_call, calls_per_sec, timesec,
	       loops_cnt, tsc_interval);

	return 0;
}

void print_result(uint64_t tsc_cycles, double ns_per_pkt, double pps,
		  double timesec, int cnt_send, uint64_t tsc_interval)
{
	if (verbose) {
		printf(" - Per packet: %lu cycles(tsc) %.2f ns, %.2f pps (time:%.2f sec)\n"
		       "   (packet count:%d tsc_interval:%lu)\n",
		       tsc_cycles, ns_per_pkt, pps, timesec,
		       cnt_send, tsc_interval);
	} else {
		printf("%.2f\t%.2f\t%lu\n", ns_per_pkt, pps, tsc_interval);
	}
}

void print_header(const char *fct, int batch)
{
	if (verbose && batch)
		printf("\nPerformance of: %s, batch size: %d\n", fct, batch);
	else if (verbose)
		printf("\nPerformance of: %s\n", fct);
	else if (batch)
		printf("%s/%-4d\t", fct, batch);
	else
		printf("%-10s\t", fct);
}

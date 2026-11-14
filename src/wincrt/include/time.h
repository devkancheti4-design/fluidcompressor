/* time.h -- just enough for code that names a monotonic clock (see wincrt.c) */
#ifndef WC_TIME_H
#define WC_TIME_H
#include <stdint.h>
struct timespec { int64_t tv_sec; long tv_nsec; };
#define CLOCK_MONOTONIC 1
int clock_gettime(int clk, struct timespec *t);
#endif

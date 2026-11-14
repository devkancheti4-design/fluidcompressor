/* plat.h -- what fluid.c asks of the machine.  plat_posix.c answers on macOS / Linux, wincrt.c on Windows. */
#ifndef PLAT_H
#define PLAT_H
#include <stddef.h>
double plat_now(void);                                       /* seconds on a monotonic clock */
size_t plat_peak(void);                                      /* peak resident memory of this process, bytes (0 = unknown) */
void plat_home(const char *argv0, char *dir, size_t cap);    /* the folder that holds this program */
void plat_machine(char *s, size_t cap);                      /* "CPU | N logical cores | M GB RAM" */
int plat_run(int n, void (*fn)(void *), void *arg);          /* run fn(arg) on n threads and wait for all of them */
#endif

/* plat_posix.c -- plat.h on macOS and Linux, over the system C library */
#define _DARWIN_C_SOURCE
#define _GNU_SOURCE
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <sys/sysctl.h>
#endif
#include "plat.h"

double plat_now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (double)t.tv_sec + (double)t.tv_nsec * 1e-9; }
size_t plat_peak(void) {
    struct rusage u; if (getrusage(RUSAGE_SELF, &u)) return 0;
#ifdef __APPLE__
    return (size_t)u.ru_maxrss;                               /* bytes on macOS */
#else
    return (size_t)u.ru_maxrss * 1024;                        /* kilobytes on Linux */
#endif
}
void plat_home(const char *argv0, char *dir, size_t cap) {
    char p[PATH_MAX], r[PATH_MAX];
#ifdef __APPLE__
    uint32_t sz = sizeof p; if (_NSGetExecutablePath(p, &sz) != 0) snprintf(p, sizeof p, "%s", argv0);
#else
    ssize_t k = readlink("/proc/self/exe", p, sizeof p - 1); if (k > 0) p[k] = 0; else snprintf(p, sizeof p, "%s", argv0);
#endif
    if (realpath(p, r)) snprintf(p, sizeof p, "%s", r);
    char *s = strrchr(p, '/'); if (s) *s = 0; else snprintf(p, sizeof p, ".");
    snprintf(dir, cap, "%s", p);
}
void plat_machine(char *s, size_t cap) {
#ifdef __APPLE__
    char b[256] = "unknown CPU"; size_t bl = sizeof b; sysctlbyname("machdep.cpu.brand_string", b, &bl, NULL, 0);
    int nc = 0; size_t nl = sizeof nc; sysctlbyname("hw.logicalcpu", &nc, &nl, NULL, 0);
    uint64_t mem = 0; size_t ml = sizeof mem; sysctlbyname("hw.memsize", &mem, &ml, NULL, 0);
    snprintf(s, cap, "%s | %d logical cores | %.1f GB RAM", b, nc, (double)mem / 1073741824.0);
#else
    char b[256] = "unknown CPU", line[512]; FILE *f = fopen("/proc/cpuinfo", "r");
    if (f) { while (fgets(line, sizeof line, f)) if (!strncmp(line, "model name", 10)) { char *c = strchr(line, ':'); if (c) { snprintf(b, sizeof b, "%s", c + 2); b[strcspn(b, "\n")] = 0; } break; } fclose(f); }
    long nc = sysconf(_SC_NPROCESSORS_ONLN), pg = sysconf(_SC_PHYS_PAGES), ps = sysconf(_SC_PAGESIZE);
    snprintf(s, cap, "%s | %ld logical cores | %.1f GB RAM", b, nc, (double)pg * (double)ps / 1073741824.0);
#endif
}
typedef struct { void (*fn)(void *); void *arg; } ThreadArg;
static void *tramp(void *p) { ThreadArg *t = p; t->fn(t->arg); return NULL; }
int plat_run(int n, void (*fn)(void *), void *arg) {
    if (n <= 1) { fn(arg); return 0; }
    if (n > 64) n = 64;
    pthread_t th[64]; ThreadArg ta = {fn, arg}; int k = 0;
    pthread_attr_t at; pthread_attr_init(&at); pthread_attr_setstacksize(&at, (size_t)8 << 20);
    for (int i = 0; i < n; i++) if (!pthread_create(&th[k], &at, tramp, &ta)) k++;
    if (!k) { fn(arg); return 0; }
    for (int i = 0; i < k; i++) pthread_join(th[i], NULL);
    return 0;
}

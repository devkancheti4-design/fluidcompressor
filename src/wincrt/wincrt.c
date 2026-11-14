/* wincrt.c -- the small C runtime fluid.exe needs, written straight onto kernel32 (no MSVC or MinGW runtime, nothing
   downloaded): program entry and argv (Windows quoting rules), heap, buffered files and console, printf, getenv, a
   monotonic clock, threads, and the machine facts the edge test reports.
   Compiled with -ffreestanding so the compiler never turns these loops back into calls to themselves. */
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "k32.h"
#if defined(__x86_64__) || defined(__i386__)
#include <cpuid.h>
#endif

int _fltused = 0x9875;                       /* MSVC-target objects that use floating point reference this */

/* ------------------------------------------------------------ memory ------------------------------------------------ */
static HANDLE HEAP;
typedef struct { size_t size; uint32_t kind, magic; } Hdr;   /* 16 bytes: keeps every block 16-byte aligned */
enum { BIG = 1 << 20, MAGIC = 0x464C5544 };
static void *wc_alloc(size_t n, int zero) {
    if (n > ((size_t)-1) / 2) return NULL;
    size_t t = n + sizeof(Hdr); Hdr *h;
    if (n >= BIG) {                                          /* large: fresh pages, already zero, committed lazily */
        h = VirtualAlloc(NULL, t, K32_MEM_COMMIT_RESERVE, K32_PAGE_READWRITE); if (!h) return NULL; h->kind = 1;
    } else {
        if (!HEAP) HEAP = GetProcessHeap();
        h = HeapAlloc(HEAP, zero ? K32_HEAP_ZERO : 0u, t); if (!h) return NULL; h->kind = 0;
    }
    h->size = n; h->magic = MAGIC; return h + 1;
}
void *malloc(size_t n) { return wc_alloc(n, 0); }
void *calloc(size_t n, size_t size) { if (size && n > ((size_t)-1) / size) return NULL; return wc_alloc(n * size, 1); }
void free(void *p) {
    if (!p) return;
    Hdr *h = (Hdr *)p - 1;
    if (h->kind == 1) VirtualFree(h, 0, K32_MEM_RELEASE); else HeapFree(HEAP, 0, h);
}
void *realloc(void *p, size_t n) {
    if (!p) return malloc(n);
    if (!n) { free(p); return NULL; }
    Hdr *h = (Hdr *)p - 1; void *q = malloc(n); if (!q) return NULL;
    memcpy(q, p, h->size < n ? h->size : n); free(p); return q;
}

/* ------------------------------------------------------------ strings ----------------------------------------------- */
void *memcpy(void *d, const void *s, size_t n) {
#if defined(__x86_64__)
    void *r = d; __asm__ volatile("rep movsb" : "+D"(d), "+S"(s), "+c"(n) : : "memory"); return r;
#else
    unsigned char *a = d; const unsigned char *b = s; while (n--) *a++ = *b++; return d;
#endif
}
void *memset(void *d, int c, size_t n) {
#if defined(__x86_64__)
    void *r = d; __asm__ volatile("rep stosb" : "+D"(d), "+c"(n) : "a"(c) : "memory"); return r;
#else
    unsigned char *a = d; while (n--) *a++ = (unsigned char)c; return d;
#endif
}
void *memmove(void *d, const void *s, size_t n) {
    unsigned char *a = d; const unsigned char *b = s;
    if (a == b || !n) return d;
    if (a < b || a >= b + n) { while (n--) *a++ = *b++; }
    else { a += n; b += n; while (n--) *--a = *--b; }
    return d;
}
int memcmp(const void *x, const void *y, size_t n) {
    const unsigned char *a = x, *b = y;
    for (; n; n--, a++, b++) if (*a != *b) return *a < *b ? -1 : 1;
    return 0;
}
void *memchr(const void *s, int c, size_t n) { const unsigned char *p = s; for (; n; n--, p++) if (*p == (unsigned char)c) return (void *)p; return NULL; }
size_t strlen(const char *s) { const char *p = s; while (*p) p++; return (size_t)(p - s); }
int strcmp(const char *a, const char *b) { while (*a && *a == *b) a++, b++; return (unsigned char)*a < (unsigned char)*b ? -1 : (unsigned char)*a > (unsigned char)*b; }
int strncmp(const char *a, const char *b, size_t n) {
    for (; n; n--, a++, b++) { if (*a != *b) return (unsigned char)*a < (unsigned char)*b ? -1 : 1; if (!*a) return 0; }
    return 0;
}
char *strchr(const char *s, int c) { for (;; s++) { if (*s == (char)c) return (char *)s; if (!*s) return NULL; } }
char *strrchr(const char *s, int c) { const char *r = NULL; for (;; s++) { if (*s == (char)c) r = s; if (!*s) return (char *)r; } }
char *strcpy(char *d, const char *s) { char *r = d; while ((*d++ = *s++)) ; return r; }
char *strcat(char *d, const char *s) { strcpy(d + strlen(d), s); return d; }
int abs(int v) { return v < 0 ? -v : v; }
static int isspace_(int c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f'; }
static unsigned long long parse_ull(const char *s, char **end, int base, int *neg) {
    const char *p = s; *neg = 0;
    while (isspace_((unsigned char)*p)) p++;
    if (*p == '+' || *p == '-') *neg = *p++ == '-';
    if ((base == 0 || base == 16) && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) { p += 2; base = 16; }
    else if (base == 0) base = p[0] == '0' ? 8 : 10;
    unsigned long long v = 0; const char *q = p;
    for (;; p++) {
        int d = *p >= '0' && *p <= '9' ? *p - '0' : *p >= 'a' && *p <= 'z' ? *p - 'a' + 10 : *p >= 'A' && *p <= 'Z' ? *p - 'A' + 10 : 99;
        if (d >= base) break;
        v = v * (unsigned)base + (unsigned)d;
    }
    if (end) *end = (char *)(p == q ? s : p);
    return v;
}
int atoi(const char *s) { int neg; unsigned long long v = parse_ull(s, NULL, 10, &neg); return (int)(neg ? 0 - v : v); }
long strtol(const char *s, char **end, int base) { int neg; unsigned long long v = parse_ull(s, end, base, &neg); return (long)(neg ? 0 - v : v); }
unsigned long strtoul(const char *s, char **end, int base) { int neg; unsigned long long v = parse_ull(s, end, base, &neg); return (unsigned long)(neg ? 0 - v : v); }

/* ------------------------------------------------------------ exit -------------------------------------------------- */
static void (*ATEXIT[16])(void); static int NATEXIT;
int atexit(void (*fn)(void)) { if (NATEXIT >= 16) return -1; ATEXIT[NATEXIT++] = fn; return 0; }
void exit(int code) {
    while (NATEXIT > 0) ATEXIT[--NATEXIT]();
    fflush(stdout); fflush(stderr);
    ExitProcess((uint32_t)code);
}
void abort(void) { fflush(stdout); fflush(stderr); ExitProcess(3); }

/* ------------------------------------------------------------ environment ------------------------------------------- */
static char ENVB[8][2048]; static int ENVI;
char *getenv(const char *name) {
    int i = __atomic_fetch_add(&ENVI, 1, __ATOMIC_RELAXED) & 7;
    DWORD r = GetEnvironmentVariableA(name, ENVB[i], (DWORD)sizeof ENVB[i]);
    if (r == 0 || r >= sizeof ENVB[i]) return NULL;
    return ENVB[i];
}

/* ------------------------------------------------------------ files ------------------------------------------------- */
struct wc_file { HANDLE h; int wr, err, eof, std; size_t bn; unsigned char buf[16384]; };
static struct wc_file F_IN = {0, 0, 0, 0, 1, 0, {0}}, F_OUT = {0, 1, 0, 0, 1, 0, {0}}, F_ERR = {0, 1, 0, 0, 1, 0, {0}};
FILE *stdin = &F_IN, *stdout = &F_OUT, *stderr = &F_ERR;
static int raw_write(HANDLE h, const unsigned char *p, size_t n) {
    while (n) {
        DWORD k = n > (1u << 30) ? (1u << 30) : (DWORD)n, w = 0;
        if (!WriteFile(h, p, k, &w, NULL) || !w) return -1;
        p += w; n -= w;
    }
    return 0;
}
int fflush(FILE *f) {
    if (!f) { int a = fflush(stdout), b = fflush(stderr); return a | b; }
    if (f->wr && f->bn) {
        size_t n = f->bn; f->bn = 0;
        if (raw_write(f->h, f->buf, n)) { f->err = 1; return EOF; }
    }
    return 0;
}
FILE *fopen(const char *path, const char *mode) {
    int w = mode[0] == 'w' || mode[0] == 'a', app = mode[0] == 'a';
    HANDLE h = CreateFileA(path, w ? K32_GENERIC_WRITE : K32_GENERIC_READ, w ? K32_SHARE_READ : (K32_SHARE_READ | K32_SHARE_WRITE), NULL,
                           w ? (app ? K32_OPEN_ALWAYS : K32_CREATE_ALWAYS) : K32_OPEN_EXISTING, K32_ATTR_NORMAL, NULL);
    if (h == K32_INVALID) return NULL;
    FILE *f = malloc(sizeof *f);
    if (!f) { CloseHandle(h); return NULL; }
    f->h = h; f->wr = w; f->err = f->eof = f->std = 0; f->bn = 0;
    if (app) { int64_t pos; SetFilePointerEx(h, 0, &pos, 2); }
    return f;
}
int fclose(FILE *f) {
    if (!f) return EOF;
    int r = fflush(f);
    if (f->std) return r;
    if (!CloseHandle(f->h)) r = EOF;
    free(f); return r;
}
size_t fwrite(const void *p, size_t size, size_t n, FILE *f) {
    size_t t = size * n; const unsigned char *s = p;
    if (!t) return 0;
    if (!f->wr) { f->err = 1; return 0; }
    if (t >= sizeof f->buf) {
        if (fflush(f)) return 0;
        if (raw_write(f->h, s, t)) { f->err = 1; return 0; }
        return n;
    }
    if (f->bn + t > sizeof f->buf && fflush(f)) return 0;
    memcpy(f->buf + f->bn, s, t); f->bn += t;
    if (f == stderr) fflush(f);                              /* stderr is unbuffered */
    return n;
}
size_t fread(void *p, size_t size, size_t n, FILE *f) {
    size_t t = size * n, got = 0; unsigned char *d = p;
    if (!t) return 0;
    while (got < t) {
        DWORD k = (t - got) > (1u << 30) ? (1u << 30) : (DWORD)(t - got), r = 0;
        if (!ReadFile(f->h, d + got, k, &r, NULL)) { f->err = 1; break; }
        if (!r) { f->eof = 1; break; }
        got += r;
    }
    return got / size;
}
int fseek(FILE *f, long off, int whence) {
    if (fflush(f)) return -1;
    int64_t pos;
    if (!SetFilePointerEx(f->h, (int64_t)off, &pos, (DWORD)whence)) return -1;
    f->eof = 0; return 0;
}
long ftell(FILE *f) {
    int64_t pos;
    if (!SetFilePointerEx(f->h, 0, &pos, 1)) return -1;
    if (f->wr) pos += (int64_t)f->bn;
    return pos > 0x7FFFFFFF ? -1 : (long)pos;                /* long is 32 bits on Windows */
}
int fputc(int c, FILE *f) { unsigned char b = (unsigned char)c; return fwrite(&b, 1, 1, f) == 1 ? b : EOF; }
int putc(int c, FILE *f) { return fputc(c, f); }
int putchar(int c) { return fputc(c, stdout); }
int fputs(const char *s, FILE *f) { size_t n = strlen(s); return fwrite(s, 1, n, f) == n || !n ? 0 : EOF; }
int puts(const char *s) { return fputs(s, stdout) == EOF || fputc('\n', stdout) == EOF ? EOF : 0; }
int remove(const char *path) { return DeleteFileA(path) ? 0 : -1; }

/* ------------------------------------------------------------ printf ------------------------------------------------ */
typedef struct { char *b; size_t cap, len; } Sink;
static void sput(Sink *s, char c) { if (s->len + 1 < s->cap) s->b[s->len] = c; s->len++; }
static void spad(Sink *s, char c, int n) { while (n-- > 0) sput(s, c); }
static int utoa_(char *o, unsigned long long u, unsigned base, int upper, int prec) {
    char t[72]; int n = 0; const char *dig = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    while (u) { t[n++] = dig[u % base]; u /= base; }
    while (n < prec) t[n++] = '0';
    if (prec < 0 && n == 0) t[n++] = '0';
    for (int i = 0; i < n; i++) o[i] = t[n - 1 - i];
    return n;
}
static int ftoa_fixed(char *o, double v, int prec) {          /* v >= 0, finite, small enough to scale */
    unsigned long long p10 = 1; for (int i = 0; i < prec; i++) p10 *= 10;
    double t = v * (double)p10; unsigned long long q = (unsigned long long)t; double fr = t - (double)q;
    if (fr > 0.5 || (fr == 0.5 && (q & 1))) q++;
    int n = utoa_(o, q / p10, 10, 0, -1);
    if (prec > 0) { o[n++] = '.'; n += utoa_(o + n, q % p10, 10, 0, prec); }
    return n;
}
static int ftoa_(char *o, double v, char cv, int prec, int *neg) {
    if (v != v) { memcpy(o, "nan", 3); return 3; }
    if (v < 0 || (v == 0 && 1 / v < 0)) { *neg = 1; v = -v; }
    if (v > 1.7976931348623157e308) { memcpy(o, "inf", 3); return 3; }
    if (prec > 17) prec = 17;
    int expo = cv == 'e' || cv == 'E';
    if ((cv == 'g' || cv == 'G') && v != 0 && (v < 1e-4 || v >= 1e15)) expo = 1;
    double p10d = 1; for (int i = 0; i < prec; i++) p10d *= 10;
    if (!expo && v * p10d >= 1.8e19) expo = 1;                  /* fixed point must fit 64 bits */
    if (!expo) {
        int n = ftoa_fixed(o, v, prec);
        if ((cv == 'g' || cv == 'G') && prec > 0) { while (n > 0 && o[n - 1] == '0') n--; if (n > 0 && o[n - 1] == '.') n--; }
        return n;
    }
    int e = 0;
    if (v != 0) { while (v >= 10) { v /= 10; e++; } while (v < 1) { v *= 10; e--; } }
    int n = ftoa_fixed(o, v, prec);
    if (o[0] == '1' && o[1] == '0' && (n == 2 || o[2] == '.')) {   /* rounded up to 10.x: renormalise */
        v /= 10; e++; n = ftoa_fixed(o, v, prec);
    }
    o[n++] = cv == 'E' || cv == 'G' ? 'E' : 'e'; o[n++] = e < 0 ? '-' : '+';
    if (e < 0) e = -e;
    if (e < 10) o[n++] = '0';
    n += utoa_(o + n, (unsigned long long)e, 10, 0, -1);
    return n;
}
static void wc_format(Sink *s, const char *fmt, va_list ap) {
    for (const char *p = fmt; *p; p++) {
        if (*p != '%') { sput(s, *p); continue; }
        p++;
        int left = 0, zero = 0, plus = 0, space = 0, alt = 0;
        for (;; p++) {
            if (*p == '-') left = 1; else if (*p == '0') zero = 1; else if (*p == '+') plus = 1;
            else if (*p == ' ') space = 1; else if (*p == '#') alt = 1; else break;
        }
        int width = 0, prec = -1;
        if (*p == '*') { width = va_arg(ap, int); if (width < 0) { left = 1; width = -width; } p++; }
        else while (*p >= '0' && *p <= '9') width = width * 10 + (*p++ - '0');
        if (*p == '.') {
            p++; prec = 0;
            if (*p == '*') { prec = va_arg(ap, int); p++; if (prec < 0) prec = -1; }
            else while (*p >= '0' && *p <= '9') prec = prec * 10 + (*p++ - '0');
        }
        int len = 0;                                          /* 0 int, 1 long, 2 long long, 3 size_t, 4 intmax, 5 ptrdiff, -1 short, -2 char */
        for (;;) {
            if (*p == 'l') { len = len == 1 ? 2 : 1; p++; }
            else if (*p == 'h') { len = len == -1 ? -2 : -1; p++; }
            else if (*p == 'z') { len = 3; p++; } else if (*p == 'j') { len = 4; p++; }
            else if (*p == 't') { len = 5; p++; } else if (*p == 'L') { p++; }
            else break;
        }
        char cv = *p; if (!cv) break;
        char tmp[400]; const char *str = tmp; int tn = 0, neg = 0, num = 1, isfloat = 0; const char *pre = "";
        switch (cv) {
        case '%': sput(s, '%'); continue;
        case 'c': tmp[0] = (char)va_arg(ap, int); tn = 1; num = 0; break;
        case 's': {
            str = va_arg(ap, const char *); if (!str) str = "(null)";
            tn = 0; while (str[tn] && (prec < 0 || tn < prec)) tn++;
            num = 0; break;
        }
        case 'd': case 'i': {
            long long v = len == 2 || len == 4 ? va_arg(ap, long long) : len == 1 ? (long long)va_arg(ap, long)
                        : len == 3 || len == 5 ? (long long)va_arg(ap, ptrdiff_t) : (long long)va_arg(ap, int);
            if (len == -1) v = (short)v; else if (len == -2) v = (signed char)v;
            unsigned long long u = v < 0 ? 0ull - (unsigned long long)v : (unsigned long long)v; neg = v < 0;
            tn = utoa_(tmp, u, 10, 0, prec); break;
        }
        case 'u': case 'x': case 'X': case 'o': {
            unsigned long long u = len == 2 || len == 4 ? va_arg(ap, unsigned long long) : len == 1 ? (unsigned long long)va_arg(ap, unsigned long)
                                 : len == 3 || len == 5 ? (unsigned long long)va_arg(ap, size_t) : (unsigned long long)va_arg(ap, unsigned);
            if (len == -1) u = (unsigned short)u; else if (len == -2) u = (unsigned char)u;
            tn = utoa_(tmp, u, cv == 'o' ? 8u : cv == 'u' ? 10u : 16u, cv == 'X', prec);
            if (alt && u && (cv == 'x' || cv == 'X')) pre = cv == 'x' ? "0x" : "0X";
            break;
        }
        case 'p': { uintptr_t u = (uintptr_t)va_arg(ap, void *); pre = "0x"; tn = utoa_(tmp, (unsigned long long)u, 16, 0, -1); break; }
        case 'f': case 'F': case 'e': case 'E': case 'g': case 'G': {
            double v = va_arg(ap, double); isfloat = 1;
            tn = ftoa_(tmp, v, cv, prec < 0 ? 6 : prec, &neg); break;
        }
        default: sput(s, '%'); sput(s, cv); continue;
        }
        char sign = 0;
        if (num) sign = neg ? '-' : (plus && (cv == 'd' || cv == 'i' || isfloat)) ? '+' : (space && (cv == 'd' || cv == 'i' || isfloat)) ? ' ' : 0;
        int pl = (int)strlen(pre), total = tn + (sign ? 1 : 0) + pl;
        int zpad = num && zero && !left && (prec < 0 || isfloat);
        if (!left && !zpad) spad(s, ' ', width - total);
        if (sign) sput(s, sign);
        for (int i = 0; i < pl; i++) sput(s, pre[i]);
        if (zpad) spad(s, '0', width - total);
        for (int i = 0; i < tn; i++) sput(s, str[i]);
        if (left) spad(s, ' ', width - total);
    }
}
int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap) {
    Sink s = {buf, n, 0}; wc_format(&s, fmt, ap);
    if (n) buf[s.len < n ? s.len : n - 1] = 0;
    return (int)s.len;
}
int snprintf(char *buf, size_t n, const char *fmt, ...) { va_list ap; va_start(ap, fmt); int r = vsnprintf(buf, n, fmt, ap); va_end(ap); return r; }
int sprintf(char *buf, const char *fmt, ...) { va_list ap; va_start(ap, fmt); int r = vsnprintf(buf, (size_t)1 << 30, fmt, ap); va_end(ap); return r; }
int vfprintf(FILE *f, const char *fmt, va_list ap) {
    char small[1024]; va_list a2; va_copy(a2, ap);
    int n = vsnprintf(small, sizeof small, fmt, ap);
    if (n < 0) { va_end(a2); return n; }
    if ((size_t)n < sizeof small) { va_end(a2); return fwrite(small, 1, (size_t)n, f) == (size_t)n || !n ? n : -1; }
    char *big = malloc((size_t)n + 1);
    if (!big) { va_end(a2); return -1; }
    vsnprintf(big, (size_t)n + 1, fmt, a2); va_end(a2);
    int r = fwrite(big, 1, (size_t)n, f) == (size_t)n ? n : -1; free(big); return r;
}
int fprintf(FILE *f, const char *fmt, ...) { va_list ap; va_start(ap, fmt); int r = vfprintf(f, fmt, ap); va_end(ap); return r; }
int printf(const char *fmt, ...) { va_list ap; va_start(ap, fmt); int r = vfprintf(stdout, fmt, ap); va_end(ap); return r; }
void perror(const char *s) {
    DWORD e = GetLastError();
    const char *m = e == 2 ? "file not found" : e == 3 ? "folder not found" : e == 5 ? "access denied" : e == 32 ? "file in use" : "cannot open";
    fprintf(stderr, "%s%s%s (Windows error %u)\n", s ? s : "", s && *s ? ": " : "", m, (unsigned)e);
}

/* ------------------------------------------------------------ time ---------------------------------------------------- */
int clock_gettime(int clk, struct timespec *t) {
    (void)clk; static int64_t fq; int64_t c;
    if (!fq) QueryPerformanceFrequency(&fq);
    QueryPerformanceCounter(&c);
    t->tv_sec = c / fq; t->tv_nsec = (long)((c % fq) * 1000000000 / fq);
    return 0;
}

/* ------------------------------------------------------------ what fluid.c asks of the machine (plat.h) -------------- */
double plat_now(void) {
    static int64_t fq; int64_t c;
    if (!fq) QueryPerformanceFrequency(&fq);
    QueryPerformanceCounter(&c);
    return (double)c / (double)fq;
}
size_t plat_peak(void) {
    K32_PROCESS_MEMORY_COUNTERS m; memset(&m, 0, sizeof m); m.cb = (DWORD)sizeof m;
    return K32GetProcessMemoryInfo((HANDLE)(intptr_t)-1, &m, (DWORD)sizeof m) ? m.peak_working_set : 0;
}
void plat_home(const char *argv0, char *dir, size_t cap) {
    char p[2048]; DWORD n = GetModuleFileNameA(NULL, p, (DWORD)sizeof p);
    if (n == 0 || n >= sizeof p) snprintf(p, sizeof p, "%s", argv0 ? argv0 : ".");
    char *a = strrchr(p, '\\'), *b = strrchr(p, '/'), *c = a > b ? a : b;
    if (c) *c = 0; else snprintf(p, sizeof p, ".");
    snprintf(dir, cap, "%s", p);
}
void plat_machine(char *s, size_t cap) {
    char brand[49] = "unknown CPU";
#if defined(__x86_64__) || defined(__i386__)
    unsigned a, b, c, d, w[12];
    __cpuid(0x80000000u, a, b, c, d);
    if (a >= 0x80000004u) {
        for (unsigned i = 0; i < 3; i++) { __cpuid(0x80000002u + i, w[4 * i], w[4 * i + 1], w[4 * i + 2], w[4 * i + 3]); }
        memcpy(brand, w, 48); brand[48] = 0;
    }
#endif
    char *t = brand; while (*t == ' ') t++;
    K32_SYSTEM_INFO si; memset(&si, 0, sizeof si); GetSystemInfo(&si);
    K32_MEMORYSTATUSEX ms; memset(&ms, 0, sizeof ms); ms.length = (DWORD)sizeof ms; GlobalMemoryStatusEx(&ms);
    snprintf(s, cap, "%s | %u logical cores | %.1f GB RAM", t, (unsigned)si.n_processors, (double)ms.total_phys / 1073741824.0);
}
typedef struct { void (*fn)(void *); void *arg; } ThreadArg;
static DWORD tramp(void *p) { ThreadArg *t = p; t->fn(t->arg); return 0; }
int plat_run(int n, void (*fn)(void *), void *arg) {
    if (n <= 1) { fn(arg); return 0; }
    if (n > 64) n = 64;
    HANDLE th[64]; ThreadArg ta = {fn, arg}; int k = 0;
    for (int i = 0; i < n; i++) { th[k] = CreateThread(NULL, (size_t)8 << 20, tramp, &ta, 0, NULL); if (th[k]) k++; }
    if (!k) { fn(arg); return 0; }
    for (int i = 0; i < k; i++) { WaitForSingleObject(th[i], K32_INFINITE); CloseHandle(th[i]); }
    return 0;
}

/* ------------------------------------------------------------ entry ------------------------------------------------- */
/* Windows passes one command line; split it the way the Microsoft C runtime does: spaces and tabs separate
   arguments, "..." groups, "" inside quotes is a literal quote, 2n backslashes before a quote give n backslashes and
   the quote acts, 2n+1 give n backslashes and a literal quote; other backslashes are literal. */
static int split_args(const char *p, char **argv, char *o) {
    int argc = 0;
    for (;;) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        if (argv) argv[argc] = o;
        argc++;
        int inq = 0;
        for (;;) {
            if (*p == '\\') {
                int nb = 0; while (*p == '\\') { nb++; p++; }
                if (*p == '"') { for (int i = 0; i < nb / 2; i++) *o++ = '\\'; if (nb & 1) { *o++ = '"'; p++; } }
                else for (int i = 0; i < nb; i++) *o++ = '\\';
                continue;
            }
            if (*p == '"') {
                if (inq && p[1] == '"') { *o++ = '"'; p += 2; continue; }
                inq = !inq; p++; continue;
            }
            if (!*p || (!inq && (*p == ' ' || *p == '\t'))) break;
            *o++ = *p++;
        }
        *o++ = 0;
    }
    return argc;
}
int main(int argc, char **argv);
void mainCRTStartup(void) {
    HEAP = GetProcessHeap();
    F_IN.h = GetStdHandle(K32_STD_INPUT); F_OUT.h = GetStdHandle(K32_STD_OUTPUT); F_ERR.h = GetStdHandle(K32_STD_ERROR);
    const char *cl = GetCommandLineA(); if (!cl) cl = "";
    size_t L = strlen(cl);
    char *buf = malloc(2 * L + 16), *scratch = malloc(2 * L + 16);
    int argc = split_args(cl, NULL, scratch);
    char **argv = malloc(sizeof(char *) * (size_t)(argc + 1));
    if (!buf || !scratch || !argv) ExitProcess(2);
    split_args(cl, argv, buf); argv[argc] = NULL; free(scratch);
    exit(main(argc, argv));
}

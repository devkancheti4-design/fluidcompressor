/* stdlib.h -- wincrt's stdlib (see wincrt.c) */
#ifndef WC_STDLIB_H
#define WC_STDLIB_H
#include <stddef.h>
#include <stdint.h>
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
void *malloc(size_t n);
void *calloc(size_t n, size_t size);
void *realloc(void *p, size_t n);
void free(void *p);
__attribute__((noreturn)) void exit(int code);
__attribute__((noreturn)) void abort(void);
int atexit(void (*fn)(void));
int atoi(const char *s);
long strtol(const char *s, char **end, int base);
unsigned long strtoul(const char *s, char **end, int base);
char *getenv(const char *name);
int abs(int v);
#endif

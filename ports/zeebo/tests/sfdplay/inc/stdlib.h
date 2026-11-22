#ifndef SFD_STDLIB_H
#define SFD_STDLIB_H
#include <stddef.h>
void *malloc(size_t n);
void *realloc(void *p, size_t n);
void free(void *p);
static inline int abs(int v) { return v < 0 ? -v : v; }
#endif

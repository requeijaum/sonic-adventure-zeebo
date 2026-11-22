/* shim mínimo para pl_mpeg.h no applet BREW (sem libc) */
#ifndef SFD_STRING_H
#define SFD_STRING_H
#include <stddef.h>
void *memset(void *d, int c, size_t n);
void *memcpy(void *d, const void *s, size_t n);
void *memmove(void *d, const void *s, size_t n);
#endif

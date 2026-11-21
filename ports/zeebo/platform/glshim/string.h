/* string.h — declarações freestanding de memória do port.
 * As definições estão em string.c (memset/memcpy). */
#ifndef SA1Z_STRING_H
#define SA1Z_STRING_H

void *memset(void *s, int c, unsigned int n);
void *memcpy(void *d, const void *s, unsigned int n);

#endif /* SA1Z_STRING_H */

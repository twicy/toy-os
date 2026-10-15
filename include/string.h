#ifndef STRING_H
#define STRING_H

#include <stdint.h>
#include <stddef.h>

/* gcc may emit calls to memcpy, memmove, memset and memcmp on its own,
 * even with -ffreestanding, so these must keep the standard semantics */
void *memcpy(void *dst, const void *src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
void *memset(void *dst, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

char *strcpy(char *dst, const char *src);
size_t strlen(const char *s);
int strcmp(const char *s1, const char *s2);
char *strchr(const char *s, int c);
char *strcat(char *dst, const char *src);

#endif

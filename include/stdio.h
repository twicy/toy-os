#ifndef STDIO_H
#define STDIO_H
#include <stdarg.h>
#include <stddef.h>

/* Receives each output character of vformat, arg is passed through unchanged */
typedef void (*putc_fn)(char c, void *arg);

/* Returns the number of characters produced */
int vformat(putc_fn putc, void *arg, const char *format, va_list ap)
    __attribute__((format(printf, 3, 0)));

/* Writes at most size - 1 chars plus a NUL, returns the untruncated length */
int vsnprintf(char *buf, size_t size, const char *format, va_list ap)
    __attribute__((format(printf, 3, 0)));
int snprintf(char *buf, size_t size, const char *format, ...)
    __attribute__((format(printf, 3, 4)));

#endif

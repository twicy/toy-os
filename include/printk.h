#ifndef PRINTK_H
#define PRINTK_H

/* format() lets gcc check the arguments against the format string */
int printk(const char *format, ...) __attribute__((format(printf, 1, 2)));

#endif

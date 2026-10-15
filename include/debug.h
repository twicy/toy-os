#ifndef DEBUG_H
#define DEBUG_H
#include <printk.h>

/* cli first: with IF set, the next IRQ wakes hlt and its handler keeps
 * printing over the panic message */
#define PANIC(...) do { \
    asm volatile ("cli"); \
    printk("PANIC at %s:%d %s(): ", __FILE__, __LINE__, __func__); \
    printk(__VA_ARGS__); \
    printk("\n"); \
    for (;;) { \
        asm volatile ("hlt"); \
    } \
} while (0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        PANIC("assertion failed: %s", #cond); \
    } \
} while (0)

#endif

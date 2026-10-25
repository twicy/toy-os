#ifndef IRQFLAGS_H
#define IRQFLAGS_H
#include <stdint.h>

#define EFLAGS_IF (1u << 9)

static inline uint32_t irq_save(void) {
    uint32_t flags;
    asm volatile ("pushfl\n\tpopl %0\n\tcli" : "=r"(flags) : : "memory");
    return flags;
}

static inline void irq_restore(uint32_t flags) {
    if (flags & EFLAGS_IF)
        asm volatile ("sti" : : : "memory");
}

static inline void irq_enable(void) {
    asm volatile ("sti" : : : "memory");
}

static inline void irq_disable(void) {
    asm volatile ("cli" : : : "memory");
}

/* sti only takes effect after the next instruction, so no IRQ can land
 * between the two: one already pending wakes the hlt instead */
static inline void safe_halt(void) {
    asm volatile ("sti\n\thlt" : : : "memory");
}

#endif

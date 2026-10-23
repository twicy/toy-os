#ifndef GDT_H
#define GDT_H
#include <stdint.h>

/* GDT indices, a selector is index << 3 | RPL
 * kernel code must stay at 1: the loader's cs and every IDT gate use selector 0x08 */
#define KERNEL_CODE_IDX 0x1
#define KERNEL_DATA_IDX 0x2
#define USER_CODE_IDX 0x3
#define USER_DATA_IDX 0x4
/* the TSS every thread runs under, and the one the double fault task gate switches to */
#define TSS_IDX 0x5
#define DOUBLEFAULT_TSS_IDX 0x6

#define KERNEL_CODE_SELECTOR (KERNEL_CODE_IDX << 3)
#define KERNEL_DATA_SELECTOR (KERNEL_DATA_IDX << 3)
#define USER_DATA_SELECTOR (USER_DATA_IDX << 3)
#define USER_CODE_SELECTOR (USER_CODE_IDX << 3)
#define TSS_SELECTOR (TSS_IDX << 3)
#define DOUBLEFAULT_TSS_SELECTOR (DOUBLEFAULT_TSS_IDX << 3)

#define MAX_GDT_ENTRIES 16

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdtr{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

void init_gdt(void);
/* point GDT entry idx at a TSS of size bytes */
void gdt_set_tss(uint32_t idx, void *tss, uint32_t size);

#endif
#ifndef TSS_H
#define TSS_H
#include <stdint.h>

/* 32-bit Task State Segment, the layout the CPU reads and writes on a
 * hardware task switch. The _h halves are reserved */
struct tss {
    uint16_t link, link_h;      /* selector of the task that switched here */
    uint32_t esp0;              /* stack for an interrupt from ring 3, with ss0 */
    uint16_t ss0, ss0_h;
    uint32_t esp1;
    uint16_t ss1, ss1_h;
    uint32_t esp2;
    uint16_t ss2, ss2_h;
    uint32_t cr3;               /* loaded on a switch to this task, never saved */
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint16_t es, es_h;
    uint16_t cs, cs_h;
    uint16_t ss, ss_h;
    uint16_t ds, ds_h;
    uint16_t fs, fs_h;
    uint16_t gs, gs_h;
    uint16_t ldt, ldt_h;
    uint16_t trap;
    uint16_t iomap_base;        /* past the limit: no I/O permission bitmap */
} __attribute__((packed));

_Static_assert(sizeof(struct tss) == 104, "struct tss must match the CPU's layout");

/* EFLAGS bit 1 always reads 1 */
#define EFLAGS_FIXED (1u << 1)

extern struct tss kernel_tss;

void init_tss(void);
void init_doublefault(void);

#endif

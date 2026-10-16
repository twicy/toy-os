#include <gdt.h>

static struct gdt_entry gdt[MAX_GDT_ENTRIES];
static struct gdtr gdtr;

void init_gdt(void) {

    gdt[0] = (struct gdt_entry){0};

    gdt[KERNEL_CODE_IDX].limit_low = 0xFFFF;
    gdt[KERNEL_CODE_IDX].base_low = 0x0000;
    gdt[KERNEL_CODE_IDX].base_middle = 0x00;
    gdt[KERNEL_CODE_IDX].access = 0x9A; // Present, Ring 0, Code Segment
    gdt[KERNEL_CODE_IDX].granularity = 0xCF; // 4K granularity, 32-bit
    gdt[KERNEL_CODE_IDX].base_high = 0x00;

    gdt[KERNEL_DATA_IDX].limit_low = 0xFFFF;
    gdt[KERNEL_DATA_IDX].base_low = 0x0000;
    gdt[KERNEL_DATA_IDX].base_middle = 0x00;
    gdt[KERNEL_DATA_IDX].access = 0x92; // Present, Ring 0, Data Segment
    gdt[KERNEL_DATA_IDX].granularity = 0xCF; // 4K granularity, 32-bit
    gdt[KERNEL_DATA_IDX].base_high = 0x00;

    gdt[USER_CODE_IDX].limit_low = 0xFFFF;
    gdt[USER_CODE_IDX].base_low = 0x0000;
    gdt[USER_CODE_IDX].base_middle = 0x00;
    gdt[USER_CODE_IDX].access = 0xFA; // Present, Ring 3, Code Segment
    gdt[USER_CODE_IDX].granularity = 0xCF; // 4K granularity, 32-bit
    gdt[USER_CODE_IDX].base_high = 0x00;

    gdt[USER_DATA_IDX].limit_low = 0xFFFF;
    gdt[USER_DATA_IDX].base_low = 0x0000;
    gdt[USER_DATA_IDX].base_middle = 0x00;
    gdt[USER_DATA_IDX].access = 0xF2; // Present, Ring 3, Data Segment
    gdt[USER_DATA_IDX].granularity = 0xCF; // 4K granularity, 32-bit
    gdt[USER_DATA_IDX].base_high = 0x00;

    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base = (uint32_t)&gdt;

    // lgdt only changes where descriptors are read from, each segment register keeps
    // the descriptor it cached when loaded, so reload them all from the new table
    // cs can only be reloaded by a far jump
    asm volatile("lgdt %0\n\t"
                 "ljmp %1, $1f\n"
                 "1:\n\t"
                 "mov %2, %%ax\n\t"
                 "mov %%ax, %%ds\n\t"
                 "mov %%ax, %%es\n\t"
                 "mov %%ax, %%fs\n\t"
                 "mov %%ax, %%gs\n\t"
                 "mov %%ax, %%ss"
                 : : "m"(gdtr), "i"(KERNEL_CODE_SELECTOR), "i"(KERNEL_DATA_SELECTOR)
                 : "eax", "memory");
}
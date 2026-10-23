#include <tss.h>
#include <gdt.h>

struct tss kernel_tss;

void init_tss(void) {
    /* Nothing switches to this task, but a hardware task switch saves the
     * outgoing state into the TSS that TR names. Without a valid one there,
     * the double fault task switch itself would fault and reset the CPU.
     * ss0 and esp0 stay 0 until there is a ring 3 to come from */
    kernel_tss.iomap_base = sizeof(struct tss);
    gdt_set_tss(TSS_IDX, &kernel_tss, sizeof(kernel_tss));
    asm volatile ("ltr %0" : : "r"((uint16_t)TSS_SELECTOR));
}

#include <printk.h>
#include <vga.h>
#include <idt.h>
#include <pic.h>
#include <timer.h>
#include <debug.h>
#include <memory.h>
#include <gdt.h>

static void on_breakpoint(struct intr_frame *f) {
    printk("int3 at eip %#x\n", f->eip);
}

int main(void) {
    init_gdt();
    clear_screen();
    printk("toy-os kernel entered at %p\n", (void *)main);
    print_memory_map();
    init_idt();
    init_pic();
    register_handler(3, on_breakpoint);
    asm volatile ("int $3");
    printk("back from int3\n");
    init_timer();
    /* IF has been 0 since the loader's cli, IRQs only arrive from here on */
    asm volatile ("sti");
    ASSERT(1 == 2);
    for (;;) {
        /* sleep until the next interrupt instead of spinning */
        asm volatile ("hlt");
    }
    return 0;
}
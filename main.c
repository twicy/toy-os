#include <printk.h>
#include <vga.h>
#include <idt.h>

static void on_breakpoint(struct intr_frame *f) {
    printk("int3 at eip %#x\n", f->eip);
}

int main(void) {
    clear_screen();
    printk("toy-os kernel entered at %p\n", (void *)main);
    init_idt();
    register_handler(3, on_breakpoint);
    asm volatile ("int $3");
    printk("back from int3\n");
    for (;;);
    return 0;
}
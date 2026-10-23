#include <printk.h>
#include <vga.h>
#include <idt.h>
#include <pic.h>
#include <timer.h>
#include <debug.h>
#include <mm/memblock.h>
#include <gdt.h>
#include <mm/paging.h>
#include <mm/page_alloc.h>
#include <mm/slab.h>
#include <mm/vmalloc.h>
#include <task.h>
#include <tss.h>

static void on_breakpoint(struct intr_frame *f) {
    printk("int3 at eip %#x\n", f->eip);
}

int main(void) {
    init_gdt();
    clear_screen();
    printk("toy-os kernel entered at %p\n", (void *)main);

    /* GDT's memory is safely pre-allcoated in BSS section, under _end */
    /* But kernel PG and PTs are phys_mem_alloc()ed already */
    e820_probe_phys_mem();
    init_pd();
    // Hand over physical pages to zone
    init_page_alloc();
    init_kmalloc();
    init_vmalloc();
    init_tasks();

    init_idt();
    init_tss();
    init_doublefault();
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
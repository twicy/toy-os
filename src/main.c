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

static void demo_thread(void *arg) {
    (void)arg;

    for (int i = 0; i < 3; i++) {
        printk("%s: round %d\n", current->name, i);
        schedule();
    }
    printk("%s: parked\n", current->name);
    /* no wake up and no exit yet: off the run queue for good */
    current->state = TASK_BLOCKED;
    schedule();
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

    kthread_create(demo_thread, NULL, "A");
    kthread_create(demo_thread, NULL, "B");
    kthread_create(demo_thread, NULL, "C");
    /* done setting up: main() is the idle thread from here on */
    cpu_idle();
}
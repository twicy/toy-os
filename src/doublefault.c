#include <tss.h>
#include <gdt.h>
#include <idt.h>
#include <task.h>
#include <mm/paging.h>
#include <debug.h>

#define DOUBLEFAULT_VEC 8

static struct tss doublefault_tss;
static uint8_t doublefault_stack[PAGE_SIZE] __attribute__((aligned(16)));

static __attribute__((noreturn)) void doublefault_task(void) {
    uint32_t cr2;
    uint32_t stack = (uint32_t)current->stack;

    /* the faulting address of the page fault that turned into this */
    asm volatile ("mov %%cr2, %0" : "=r"(cr2));

    if (stack && cr2 < stack && cr2 >= stack - PAGE_SIZE) {
        PANIC("kernel stack overflow in %s (pid %u): eip %#x esp %#x, the stack starts at %#x",
            current->name, current->pid, kernel_tss.eip, kernel_tss.esp, stack);
    }
    PANIC("double fault in %s (pid %u): eip %#x esp %#x cr2 %#x",
        current->name, current->pid, kernel_tss.eip, kernel_tss.esp, cr2);
}

void init_doublefault(void) {
    struct tss *t = &doublefault_tss;

    t->eip = (uint32_t)doublefault_task;
    t->esp = (uint32_t)doublefault_stack + sizeof(doublefault_stack);
    /* IF clear: nothing interrupts the report */
    t->eflags = EFLAGS_FIXED;
    t->cs = KERNEL_CODE_SELECTOR;
    t->ss = t->ds = t->es = t->fs = t->gs = KERNEL_DATA_SELECTOR;
    /* the kernel half is the same in every page directory, kernel_pd will do */
    t->cr3 = virt_to_phys(kernel_pd);
    t->iomap_base = sizeof(struct tss);

    gdt_set_tss(DOUBLEFAULT_TSS_IDX, t, sizeof(*t));
    set_task_gate(DOUBLEFAULT_VEC, DOUBLEFAULT_TSS_SELECTOR);
}

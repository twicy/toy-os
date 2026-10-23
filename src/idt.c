#include <idt.h>
#include <pic.h>
#include <printk.h>

static idt_entry_t idt[IDT_SIZE];

extern uint32_t intr_stubs[IDT_SIZE];
static intr_handler_t intr_handlers[IDT_SIZE];

static const char *const exception_names[32] = {
    [0] = "divide error",
    [1] = "debug",
    [2] = "NMI",
    [3] = "breakpoint",
    [4] = "overflow",
    [5] = "bound range exceeded",
    [6] = "invalid opcode",
    [7] = "device not available",
    [8] = "double fault",
    [9] = "coprocessor segment overrun",
    [10] = "invalid TSS",
    [11] = "segment not present",
    [12] = "stack-segment fault",
    [13] = "general protection",
    [14] = "page fault",
    [16] = "x87 floating-point",
    [17] = "alignment check",
    [18] = "machine check",
    [19] = "SIMD floating-point",
    [20] = "virtualization",
    [21] = "control protection",
};

void init_idt(void) {
    for (int i = 0; i < IDT_SIZE; i++) {
        idt[i].offset_15_0 = intr_stubs[i] & 0xFFFF;
        /* Index + TI + DPL */
        idt[i].segment_selector = 0b1000;
        idt[i].zero = 0;
        idt[i].type_attr = INTER_DESC_TYPE_DEFAULT;
        idt[i].offset_31_16 = intr_stubs[i] >> 16;
    }
    struct idt_ptr idtr;
    idtr.limit = sizeof(idt) - 1;
    idtr.base = (uint32_t)idt;
    asm volatile ("lidt %0" : : "m" (idtr));
}

void register_handler(uint8_t vec, intr_handler_t handler) {
    intr_handlers[vec] = handler;
}

void set_task_gate(uint8_t vec, uint16_t tss_selector) {
    idt[vec].offset_15_0 = 0;
    idt[vec].segment_selector = tss_selector;
    idt[vec].zero = 0;
    idt[vec].type_attr = TASK_GATE_TYPE;
    idt[vec].offset_31_16 = 0;
}

static void unhandled_intr(struct intr_frame *f) {
    if (f->vector < 32) {
        const char *name = exception_names[f->vector];
        printk("exception %u (%s), err %#x, eip %#x\n",
               f->vector, name ? name : "reserved", f->err_code, f->eip);
    } else {
        printk("unhandled vector %u, eip %#x\n", f->vector, f->eip);
    }

    for (;;)
        asm volatile ("hlt");
}

void intr_dispatch(struct intr_frame *f) {
    /* Fall thru APIC vector range */
    if (f->vector >= PIC_IRQ_BASE && f->vector < PIC_IRQ_BASE + PIC_IRQ_COUNT) {
        if (!pic_ack(f->vector - PIC_IRQ_BASE)) {
            return;
        }
    }

    if (intr_handlers[f->vector]) {
        intr_handlers[f->vector](f);
    } else {
        unhandled_intr(f);
    }
}
#include <idt.h>
#include <printk.h>

static idt_entry_t idt[IDT_SIZE];

extern uint32_t intr_stubs[IDT_SIZE];
static intr_handler_t intr_handlers[IDT_SIZE];

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

static void unhandled_intr(struct intr_frame *f) {
    printk("unhandled vector %u, err %#x, eip %#x\n", f->vector, f->err_code, f->eip);
    for (;;)
        asm volatile ("hlt");
}

void intr_dispatch(struct intr_frame *f) {
    if (intr_handlers[f->vector]) {
        intr_handlers[f->vector](f);
    } else {
        unhandled_intr(f);
    }
}
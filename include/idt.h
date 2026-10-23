#ifndef IDT_H
#define IDT_H
#include <stdint.h>

#define IDT_SIZE 256

/* With only Interrupt Gate Descriptors, nested interrupts cannot happen
 * Trap Gate Descriptors does not clear IF so interrupts are not masked
 */
struct interrupt_descriptor {
    uint16_t offset_15_0;
    uint16_t segment_selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_31_16;
} __attribute__((packed));

/* P = 1, DPL = 00b, Type = 1110b */
/* Setting DPL = 00b means, if process attempts to issue illegal interrupts
 * the control units will check CPL == 3 but DPL == 0, and deny the request.
 * A "General protection" execption will be generated.
 * Of course, syscalls are legal interrupts with DPL = 3.
 */
#define INTER_DESC_TYPE_DEFAULT 0x8E

/* P = 1, DPL = 00b, Type = 0101b: a task gate. The offset is unused, the
 * CPU switches to the task whose TSS the selector names instead of calling
 * a handler on the current stack */
#define TASK_GATE_TYPE 0x85

struct intr_frame {
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
    uint32_t vector, err_code;
    uint32_t eip, cs, eflags;
    uint32_t user_esp, user_ss; 
};

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

typedef struct interrupt_descriptor idt_entry_t;
typedef void (*intr_handler_t)(struct intr_frame *f);

void init_idt();
void register_handler(uint8_t vec, intr_handler_t handler);
void set_task_gate(uint8_t vec, uint16_t tss_selector);

#endif

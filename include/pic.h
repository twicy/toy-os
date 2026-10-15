#ifndef PIC_H
#define PIC_H
#include <stdint.h>

/* First 32 IDT vectors are reserved for exceptions */
#define PIC_IRQ_BASE 0x20
#define PIC_IRQ_COUNT 16

#define IRQ_TIMER 0
#define IRQ_KEYBOARD 1
#define IRQ_CASCADE 2

void init_pic(void);
int pic_ack(uint8_t irq);
void pic_unmask(uint8_t irq);
void pic_mask(uint8_t irq);

#endif

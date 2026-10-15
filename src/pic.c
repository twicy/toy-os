#include <io.h>
#include <pic.h>

#define PIC_MASTER_CMD 0x20
#define PIC_MASTER_DATA 0x21
#define PIC_SLAVE_CMD 0xA0
#define PIC_SLAVE_DATA 0xA1

/* ICW1: bit 4 = start init, bit 0 = ICW4 follows,
 * bit 1 = 0 cascaded, bit 3 = 0 edge triggered */
#define ICW1_INIT_ICW4 0x11
/* ICW4: bit 0 = 8086 mode, bit 1 = 0 the handler sends EOI itself */
#define ICW4_8086 0x01
/* OCW2: non-specific EOI, clears the highest priority bit in ISR */
#define OCW2_EOI 0x20
/* OCW3: the next read of the command port returns ISR instead of IRR */
#define OCW3_READ_ISR 0x0B

/* Real 8259As need a moment between ICWs, a write to the unused
 * POST code port 0x80 takes about a microsecond */
static inline void io_wait(void) {
    outb(0x80, 0);
}

void init_pic(void) {
    /* ICW1 on the command port, ICW2-4 then follow on the data port */
    outb(PIC_MASTER_CMD, ICW1_INIT_ICW4);
    io_wait();
    outb(PIC_SLAVE_CMD, ICW1_INIT_ICW4);
    io_wait();

    /* ICW2: vector of the chip's IRQ 0, the low 3 bits must be 0 */
    outb(PIC_MASTER_DATA, PIC_IRQ_BASE);
    io_wait();
    outb(PIC_SLAVE_DATA, PIC_IRQ_BASE + 8);
    io_wait();

    /* ICW3: the master gets a bitmask of the pins with a slave on them,
     * the slave gets the number of the master pin it hangs off */
    outb(PIC_MASTER_DATA, 1 << IRQ_CASCADE);
    io_wait();
    outb(PIC_SLAVE_DATA, IRQ_CASCADE);
    io_wait();

    outb(PIC_MASTER_DATA, ICW4_8086);
    io_wait();
    outb(PIC_SLAVE_DATA, ICW4_8086);
    io_wait();

    /* OCW1: mask everything but the cascade, drivers unmask their own IRQ */
    outb(PIC_MASTER_DATA, (uint8_t)~(1 << IRQ_CASCADE));
    outb(PIC_SLAVE_DATA, 0xFF);
}

/* Send EOI for an IRQ, the PIC holds back IRQs of equal or lower
 * priority until it gets one. Returns 0 if the IRQ was spurious: when a
 * line drops before the CPU acknowledges it, the PIC still has to answer
 * with some vector and hands out its lowest priority one, IRQ 7 (master)
 * or IRQ 15 (slave), without setting its bit in the in-service register.
 */
int pic_ack(uint8_t irq) {
    if (irq == 7 || irq == 15) {
        uint16_t cmd = irq == 7 ? PIC_MASTER_CMD : PIC_SLAVE_CMD;
        outb(cmd, OCW3_READ_ISR);
        if (!(inb(cmd) & 0x80)) {
            /* The master did see the cascade IRQ 2, only the slave's was fake */
            if (irq == 15) {
                outb(PIC_MASTER_CMD, OCW2_EOI);
            }
            return 0;
        }
    }
    /* An IRQ from the slave is in service on both chips */
    if (irq >= 8) {
        outb(PIC_SLAVE_CMD, OCW2_EOI);
    }
    outb(PIC_MASTER_CMD, OCW2_EOI);
    return 1;
}

/* Reading the data port returns the mask register (IMR) */
void pic_unmask(uint8_t irq) {
    uint16_t port = irq < 8 ? PIC_MASTER_DATA : PIC_SLAVE_DATA;
    outb(port, inb(port) & ~(1 << (irq % 8)));
}

void pic_mask(uint8_t irq) {
    uint16_t port = irq < 8 ? PIC_MASTER_DATA : PIC_SLAVE_DATA;
    outb(port, inb(port) | (1 << (irq % 8)));
}

#include <idt.h>
#include <io.h>
#include <pic.h>
#include <printk.h>
#include <timer.h>

/* The 8253/8254 PIT counts down from a divisor at 1.193182 MHz,
 * channel 0 raises IRQ 0 each time it reaches 0 */
#define PIT_INPUT_HZ 1193182
#define PIT_CHANNEL0 0x40
#define PIT_CMD 0x43
/* Channel 0 (bits 7-6 = 00), low byte then high byte (bits 5-4 = 11),
 * mode 2 rate generator (bits 3-1 = 010), binary count (bit 0 = 0) */
#define PIT_CMD_CH0_RATE 0x34

/* volatile: an IRQ changes it behind the back of any loop reading it */
static volatile uint32_t ticks;

static void on_timer(struct intr_frame *f) {
    (void)f;
    ticks++;
    if (ticks % TIMER_HZ == 0) {
        printk("uptime %u s\n", ticks / TIMER_HZ);
    }
}

void init_timer(void) {
    uint16_t divisor = PIT_INPUT_HZ / TIMER_HZ;

    outb(PIT_CMD, PIT_CMD_CH0_RATE);
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, divisor >> 8);

    register_handler(PIC_IRQ_BASE + IRQ_TIMER, on_timer);
    pic_unmask(IRQ_TIMER);
}

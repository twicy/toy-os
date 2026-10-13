#include <printk.h>
#include <vga.h>

int main(void) {
    clear_screen();
    printk("toy-os kernel entered at %p\n", (void *)main);
    while(1);
    return 0;
}

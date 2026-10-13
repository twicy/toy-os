#include <printk.h>
#include <stdio.h>
#include <vga.h>

static void vga_putc(char c, void *arg) {
    (void)arg;
    put_char(c, VGA_COLOR_DEFAULT);
}

/* Characters go straight to the screen, so no buffer limits the length */
int printk(const char *format, ...) {
    va_list ap;
    int len;

    va_start(ap, format);
    len = vformat(vga_putc, NULL, format, ap);
    va_end(ap);
    return len;
}

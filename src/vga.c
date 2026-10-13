#include <io.h>
#include <vga.h>

/* 0xB8000 seen through the kernel alias the loader maps at 0xC0000000 */
#define VIDEO_SEG_BASE 0xc00b8000

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

int16_t cursor_x = 0;
int16_t cursor_y = 0;

static void move_cursor() {
    uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void clear_screen() {
    uint16_t *video_memory = (uint16_t *)VIDEO_SEG_BASE;
    uint8_t color = VGA_COLOR_DEFAULT;
    uint16_t blank = (color << 8) | ' ';

    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        video_memory[i] = blank;
    }

    cursor_x = 0;
    cursor_y = 0;

    move_cursor();
}

void put_char(char c, uint8_t color) {
    uint16_t *video_memory = (uint16_t *)VIDEO_SEG_BASE;

    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else {
        // char is signed, without the cast bytes >= 0x80 sign-extend over the color
        video_memory[cursor_y * VGA_WIDTH + cursor_x] = (color << 8) | (uint8_t)c;
        cursor_x++;
    }

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= VGA_HEIGHT) {
        // Scroll the screen up
        for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
            video_memory[i] = video_memory[i + VGA_WIDTH];
        }
        // Clear the last line
        uint16_t blank = (VGA_COLOR_DEFAULT << 8) | ' ';
        for (int i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++) {
            video_memory[i] = blank;
        }
        cursor_y = VGA_HEIGHT - 1;
    }

    move_cursor();
}

void put_string(const char *str, uint8_t color) {
    while (*str) {
        put_char(*str++, color);
    }
}

void put_string_default(const char *str) {
    put_string(str, VGA_COLOR_DEFAULT);
}
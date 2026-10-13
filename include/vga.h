#ifndef VGA_H
#define VGA_H
#include <stdint.h>

#define COLOR_BLACK 0
#define COLOR_BLUE 1
#define COLOR_GREEN 2
#define COLOR_CYAN 3
#define COLOR_RED 4
#define COLOR_FUCHSINE 5
#define COLOR_BROWN 6
#define COLOR_WHITE 7

#define COLOR_LIGHT_BLACK 8
#define COLOR_LIGHT_BLUE 9
#define COLOR_LIGHT_GREEN 10
#define COLOR_LIGHT_CYAN 11
#define COLOR_LIGHT_RED 12
#define COLOR_LIGHT_FUCHSINE 13
#define COLOR_LIGHT_BROWN 14
#define COLOR_LIGHT_WHITE 15

#define VGA_COLOR_FG_DEFAULT COLOR_LIGHT_WHITE
#define VGA_COLOR_BG_DEFAULT COLOR_BLACK
#define VGA_COLOR(fg, bg) (((bg) << 4) | (fg))
#define VGA_COLOR_DEFAULT VGA_COLOR(VGA_COLOR_FG_DEFAULT, VGA_COLOR_BG_DEFAULT)

void clear_screen(void);
void put_char(char c, uint8_t color);
void put_string(const char *str, uint8_t color);
void put_string_default(const char *str);

#endif
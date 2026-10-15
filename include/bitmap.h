#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>

struct bitmap {
    uint8_t *bits;
    uint32_t nbits;
};

void bitmap_init(struct bitmap *bm, uint8_t *bits, uint32_t nbits);
void bitmap_set(struct bitmap *bm, uint32_t index, int value);
int bitmap_get(struct bitmap *bm, uint32_t index);

#endif

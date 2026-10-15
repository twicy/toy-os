#include <bitmap.h>
#include <debug.h>
#include <string.h>

void bitmap_init(struct bitmap *bm, uint8_t *bits, uint32_t nbits) {
    bm->bits = bits;
    bm->nbits = nbits;
    memset(bits, 0, (nbits + 7) / 8);
}

/* An index past the end is a caller bug, fail loudly instead of
 * silently ignoring it */
void bitmap_set(struct bitmap *bm, uint32_t index, int value) {
    ASSERT(index < bm->nbits);
    if (value) {
        bm->bits[index / 8] |= 1 << (index % 8);
    } else {
        bm->bits[index / 8] &= ~(1 << (index % 8));
    }
}

int bitmap_get(struct bitmap *bm, uint32_t index) {
    ASSERT(index < bm->nbits);
    return (bm->bits[index / 8] >> (index % 8)) & 1;
}

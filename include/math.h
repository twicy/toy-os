#ifndef MATH_H
#define MATH_H

#define is_pow2(x) !!((x) && !((x) & ((x) - 1)))
#define ALIGN_UP(start, align) (((start) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(start, align) ((start) & ~((align) - 1))
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#endif
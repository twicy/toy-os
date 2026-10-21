#ifndef KERNEL_H
#define KERNEL_H
#include <stdint.h>

/* the kernel half starts here, physical address p is at KERNEL_OFFSET + p */
#define KERNEL_OFFSET 0xC0000000

/* kernel half layout:
 * [0xC0000000, 0xF8000000)  direct map of RAM, so at most 896 MiB of it is used
 * [0xF8000000, 0xFFC00000)  vmalloc, mapped page by page
 * [0xFFC00000, 4 GiB)       never mapped
 */
#define VMALLOC_START 0xF8000000
#define VMALLOC_END 0xFFC00000

#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)
#define phys_to_virt(x) ((void *)((uintptr_t)(x) + KERNEL_OFFSET))
#define virt_to_phys(x) ((uintptr_t)(x) - KERNEL_OFFSET)

#define PFN_UP(a)   (((a) + PAGE_SIZE - 1) >> PAGE_SHIFT)
#define PFN_DOWN(a) ((a) >> PAGE_SHIFT)

#endif

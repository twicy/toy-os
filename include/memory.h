#ifndef MEMORY_H
#define MEMORY_H
#include <kernel.h>
#include <stdint.h>

#define MEM_MAP_ADDR (0x500 + KERNEL_OFFSET)
#define MEM_MAP_COUNT_ADDR (MEM_MAP_ADDR)
#define MEM_MAP_ENTRIES_ADDR (MEM_MAP_ADDR + 4)
#define MEM_MAP_ENTRY_SIZE 20
#define MEM_MAP_MAX_ENTRIES 32

#define E820_ARDS_TYPE_Memory 1
#define E820_ARDS_TYPE_Reserved 2

struct addr_range_desc {
    uint32_t base_addr_low;
    uint32_t base_addr_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
};


#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)
#define phys_to_virt(x) ((void *)((uintptr_t)(x) + KERNEL_OFFSET))
#define virt_to_phys(x) ((uintptr_t)(x) - KERNEL_OFFSET)

#define PFN_UP(a)   (((a) + PAGE_SIZE - 1) >> PAGE_SHIFT)
#define PFN_DOWN(a) ((a) >> PAGE_SHIFT)

/* end of the kernel image (.bss included), defined by ld */
extern char _end[];

/* [start, end), end is exclusive */
struct phys_mem_block {
    uint32_t start;
    uint32_t end;
};

/* highest page-aligned exclusive end a uint32_t can hold, 4 GiB itself does not fit */
#define PHYS_ADDR_LIMIT 0xFFFFF000ULL

/* capacity of the usable and the reserved block lists */
#define PHYS_MEM_MAX_BLOCKS 32

/* IVT, BDA, E820 map, loader, kernel image, ELF buffer, kernel stack, EBDA, VGA and ROM all live below */
#define LOW_MEM_END 0x100000
/* page directory and page tables the loader set up: PAGE_DIR_BASE + PURGE_LEN in boot.inc and loader.S */
#define LOADER_PAGE_TABLES_START 0x100000
#define LOADER_PAGE_TABLES_END 0x501000

/* number of page frames, one past the highest usable one */
extern uint32_t max_pfn;

void e820_probe_phys_mem(void);
void dump_phys_mem(void);
void phys_mem_reserve(uint32_t start, uint32_t end);
/* give back [start, end) reserved earlier, a range that was not reserved is ignored */
void phys_mem_free(uint32_t start, uint32_t end);
uint32_t phys_mem_alloc(uint32_t size, uint32_t align);

#endif
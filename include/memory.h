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

void print_memory_map(void);

#endif
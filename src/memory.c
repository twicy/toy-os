#include <memory.h>
#include <printk.h>

static struct addr_range_desc *mem_map = (struct addr_range_desc *)MEM_MAP_ENTRIES_ADDR;

void print_memory_map(void) {
    uint32_t count = *(uint32_t *)MEM_MAP_COUNT_ADDR;
    printk("Memory map (%u entries):\n", count);
    for (uint32_t i = 0; i < count; i++) {
        struct addr_range_desc *entry = &mem_map[i];
        printk("Entry %u: base_addr=0x%08x%08x, length=0x%08x%08x, type=%u\n",
               i,
               entry->base_addr_high, entry->base_addr_low,
               entry->length_high, entry->length_low,
               entry->type);
    }
}
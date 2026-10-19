#include <mm/memblock.h>
#include <printk.h>
#include <debug.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

static struct addr_range_desc *e820_map = (struct addr_range_desc *)MEM_MAP_ENTRIES_ADDR;

/* ascending and non-overlapping */
struct phys_mem_list {
    struct phys_mem_block blocks[PHYS_MEM_MAX_BLOCKS];
    uint32_t count;
};

/* usable RAM from E820, filled once by e820_probe_phys_mem and never changed after */
static struct phys_mem_list ards_mem;
/* free mem = ards_mem - ards_rsved, will only be used once */
static struct phys_mem_list ards_rsved;

uint32_t max_pfn;

/* insert [start, end), keeping it ascending and non-overlapping */
static void block_list_add(struct phys_mem_list *list, uint32_t start, uint32_t end) {
    struct phys_mem_block *b = list->blocks;
    uint32_t i = 0;
    uint32_t j;

    if (start >= end) {
        return;
    }

    while (i < list->count && b[i].end < start) {
        i++;
    }

    for (j = i; j < list->count && b[j].start <= end; j++) {
        start = MIN(start, b[j].start);
        end = MAX(end, b[j].end);
    }

    if (j == i) {
        if (list->count == PHYS_MEM_MAX_BLOCKS) {
            PANIC("no room for phys mem block %#x - %#x", start, end);
        }
        memmove(&b[i + 1], &b[i], (list->count - i) * sizeof(*b));
        list->count++;
    } else {
        memmove(&b[i + 1], &b[j], (list->count - j) * sizeof(*b));
        list->count -= j - i - 1;
    }
    b[i] = (struct phys_mem_block){ .start = start, .end = end };
}

/* remove [start, end), trimming or splitting the blocks it overlaps */
static void block_list_remove(struct phys_mem_list *list, uint32_t start, uint32_t end) {
    struct phys_mem_block *b = list->blocks;
    uint32_t i = 0;

    if (start >= end) {
        return;
    }

    while (i < list->count) {
        if (b[i].end <= start || end <= b[i].start) {
            // no overlap
            i++;
        } else if (start <= b[i].start && b[i].end <= end) {
            // fully covered, delete block i and look at the one moved into slot i
            memmove(&b[i], &b[i + 1], (list->count - i - 1) * sizeof(*b));
            list->count--;
        } else if (b[i].start < start && end < b[i].end) {
            // strictly inside, split into [b.start, start) and [end, b.end)
            if (list->count == PHYS_MEM_MAX_BLOCKS) {
                PANIC("no room to split phys mem block %#x - %#x", b[i].start, b[i].end);
            }
            memmove(&b[i + 2], &b[i + 1], (list->count - i - 1) * sizeof(*b));
            b[i + 1] = (struct phys_mem_block){ .start = end, .end = b[i].end };
            b[i].end = start;
            list->count++;
            i += 2;
        } else if (b[i].start < start) {
            // covers the tail of block i
            b[i].end = start;
            i++;
        } else {
            // covers the head of block i
            b[i].start = end;
            i++;
        }
    }
}

static struct phys_mem_block *block_list_overlap(struct phys_mem_list *list,
                                                 uint32_t start, uint32_t end) {
    for (uint32_t i = 0; i < list->count; i++) {
        struct phys_mem_block *b = &list->blocks[i];
        if (b->start < end && start < b->end) {
            return b;
        }
    }
    return NULL;
}

void e820_probe_phys_mem(void) {
    uint32_t count = *(uint32_t *)MEM_MAP_COUNT_ADDR;
    struct addr_range_desc *entry;

    for (uint32_t i = 0; i < count; i++) {
        entry = &e820_map[i];
        // 64 bit: the BIOS area entry 0xfffc0000 + 0x40000 = 0x100000000 wraps to 0 in 32 bits
        uint64_t start64 = ((uint64_t)entry->base_addr_high << 32) | entry->base_addr_low;
        uint64_t end64 = start64 + (((uint64_t)entry->length_high << 32) | entry->length_low);

        if (end64 < start64) {
            PANIC("E820 entry %u wraps past 2^64, memory map is broken", i);
        }

        if (entry->type != E820_ARDS_TYPE_Memory || start64 >= PHYS_ADDR_LIMIT) {
            continue;
        }

        if (end64 > PHYS_ADDR_LIMIT) {
            end64 = PHYS_ADDR_LIMIT;
        }

        uint32_t entry_start = ALIGN_UP((uint32_t)start64, PAGE_SIZE);
        uint32_t entry_end = ALIGN_DOWN((uint32_t)end64, PAGE_SIZE);

        block_list_add(&ards_mem, entry_start, entry_end);
    }

    if (ards_mem.count == 0) {
        PANIC("no usable memory in the E820 map");
    }

    max_pfn = PFN_DOWN(ards_mem.blocks[ards_mem.count - 1].end);

    // kernel occupied memory not necessarily smaller than kernel ELF size
    // static bufs in .bss takes no space in ELf
    phys_mem_reserve(0, MAX(LOW_MEM_END, virt_to_phys(_end)));
    phys_mem_reserve(LOADER_PAGE_TABLES_START, LOADER_PAGE_TABLES_END);
}

void dump_phys_mem(void) {
    printk("usable memory:\n");
    for (uint32_t i = 0; i < ards_mem.count; i++) {
        printk("  [0x%08x, 0x%08x)\n", ards_mem.blocks[i].start, ards_mem.blocks[i].end);
    }
    printk("reserved:\n");
    for (uint32_t i = 0; i < ards_rsved.count; i++) {
        printk("  [0x%08x, 0x%08x)\n", ards_rsved.blocks[i].start, ards_rsved.blocks[i].end);
    }
    printk("max_pfn: %#x\n", max_pfn);
}

void phys_mem_reserve(uint32_t start, uint32_t end) {
    block_list_add(&ards_rsved, start, end);
}

void phys_mem_free(uint32_t start, uint32_t end) {
    block_list_remove(&ards_rsved, start, end);
}

void phys_mem_for_each_free(void (*fn)(uint32_t start, uint32_t end)) {
    for (uint32_t i = 0; i < ards_mem.count; i++) {
        uint32_t start = ards_mem.blocks[i].start;
        uint32_t end = ards_mem.blocks[i].end;

        // both lists are ascending, walk the reserved blocks across this usable block
        for (uint32_t j = 0; j < ards_rsved.count && start < end; j++) {
            struct phys_mem_block *r = &ards_rsved.blocks[j];
            if (r->end <= start) {
                continue;
            }
            if (r->start >= end) {
                break;
            }
            // the gap before r
            if (r->start > start) {
                fn(start, r->start);
            }
            start = r->end;
        }
        // the tail after the last reserved block
        if (start < end) {
            fn(start, end);
        }
    }
}

uint32_t phys_mem_alloc(uint32_t size, uint32_t align) {
    if (size == 0 || !is_pow2(align)) {
        printk("%s requires size > 0 (getting %u) and align (getting %u) to be power of 2\n",
            __func__, size, align);
        return 0;
    }

    for (uint32_t i = 0; i < ards_mem.count; i++) {
        uint32_t end = ards_mem.blocks[i].end;
        uint32_t cand = ALIGN_UP(ards_mem.blocks[i].start, align);

        while (cand < end && size <= end - cand) {
            struct phys_mem_block *hit = block_list_overlap(&ards_rsved, cand, cand + size);
            if (hit == NULL) {
                phys_mem_reserve(cand, cand + size);
                return cand;
            }

            uint32_t next = ALIGN_UP(hit->end, align);
            if (next <= cand) {
                break;
            }
            cand = next;
        }
    }
    return 0;
}
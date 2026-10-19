#include <mm/page_alloc.h>
#include <mm/memblock.h>
#include <debug.h>
#include <printk.h>
#include <math.h>

/* the struct page of every frame, mem_map[pfn] */
struct page *mem_map;

static struct zone zone;

/* This page has to be the front page in its block */
static void add_to_zone(struct page *page, uint32_t order) {
    page->flags |= PG_buddy;
    page->order = order;
    list_add(&page->lru, &zone.free_blocks[order].free_list);
    zone.free_blocks[order].nr_free++;
}

static void remove_from_zone(struct page *page, uint32_t order) {
    list_del_init(&page->lru);
    page->flags &= ~PG_buddy;
    page->order = 0;
    zone.free_blocks[order].nr_free--;
}

/* Flipping the bit is a clever trick! */
static struct page *neigh_buddy(struct page *page, uint32_t order) {
    uint32_t buddy_pfn = page_to_pfn(page) ^ (1u << order);
    return buddy_pfn < max_pfn ? pfn_to_page(buddy_pfn) : NULL;
}

/* The three rules of becoming buddies:
 * 1. Both blocks have the same size (order)
 * 2. Located in contiguous physical address (pfn)
 * 3. The physical address of the first page needs to be aligned with order + 1
 */
static void free_one_page(struct page *page, uint32_t order) {
    while (order < MAX_ORDER) {
        struct page *buddy = neigh_buddy(page, order);

        if (!buddy || !(buddy->flags & PG_buddy) || buddy->order != order)
            break;
        remove_from_zone(buddy, order);
        /* the merged block starts at the lower half */
        if (buddy < page)
            page = buddy;
        order++;
    }
    add_to_zone(page, order);
}

struct page *alloc_pages(uint32_t order) {
    for (uint32_t o = order; o <= MAX_ORDER; o++) {
        struct free_block *free_block = &zone.free_blocks[o];
        if (list_empty(&free_block->free_list))
            continue;

        struct page *page = list_first_entry(&free_block->free_list, struct page, lru);
        remove_from_zone(page, o);
        while (o > order) {
            o--;
            add_to_zone(page + (1u << o), o);
        }
        page->refcount = 1;
        zone.free_pages -= (1u << order);
        return page;
    }
    return NULL;
}

void free_pages(struct page *page, uint32_t order) {
    uint32_t pfn = page_to_pfn(page);

    ASSERT(order <= MAX_ORDER);
    ASSERT(pfn < max_pfn && IS_ALIGNED(pfn, 1u << order));
    ASSERT(!(page->flags & (PG_reserved | PG_buddy)));
    ASSERT(page->refcount == 1);

    page->refcount = 0;
    zone.free_pages += (1u << order);
    free_one_page(page, order);
}

/* Only those page frames entirely contained are reclaimed */
static void free_range(uint32_t start, uint32_t end) {
    uint32_t pfn = PFN_UP(start);
    uint32_t end_pfn = PFN_DOWN(end);

    while (pfn < end_pfn) {
        /* __builtin_ctz = count trailing zeros */
        uint32_t order = pfn ? MIN((uint32_t)__builtin_ctz(pfn), MAX_ORDER) : MAX_ORDER;
        while (pfn + (1u << order) > end_pfn)
            order--;
        for (uint32_t i = 0; i < (1u << order); i++)
            pfn_to_page(pfn + i)->flags &= ~PG_reserved;
        zone.free_pages += (1u << order);
        free_one_page(pfn_to_page(pfn), order);
        pfn += 1u << order;
    }
}

void init_page_alloc(void) {
    uint32_t size = max_pfn * sizeof(struct page);
    uint32_t phys = phys_mem_alloc(size, PAGE_SIZE);

    if (phys == 0)
        PANIC("no physical memory for mem_map (%u bytes)", size);

    mem_map = phys_to_virt(phys);

    /* every frame starts reserved: holes, the kernel, page tables and mem_map itself
     * must never be handed out, only usable minus reserved is freed below */
    for (uint32_t pfn = 0; pfn < max_pfn; pfn++) {
        struct page *page = pfn_to_page(pfn);
        page->flags = PG_reserved;
        page->order = 0;
        page->refcount = 0;
        INIT_LIST_HEAD(&page->lru);
    }

    for (uint32_t o = 0; o <= MAX_ORDER; o++) {
        INIT_LIST_HEAD(&zone.free_blocks[o].free_list);
        zone.free_blocks[o].nr_free = 0;
    }
    zone.free_pages = 0;

    /* memblock is not used after this, from here on frames come from alloc_pages */
    phys_mem_for_each_free(free_range);
    printk("page allocator: %u free pages, mem_map at phys %#x\n", zone.free_pages, phys);
}
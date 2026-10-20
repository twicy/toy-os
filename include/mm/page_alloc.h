#ifndef PAGE_ALLOC_H
#define PAGE_ALLOC_H
#include <stdint.h>
#include <list.h>
#include <kernel.h>

#define PG_reserved (1u << 0)
/* Set if it is the first page of a free block in buddy system */
#define PG_buddy (1u << 1)
/* Set on every page that is a slab, the slab fields below are valid */
#define PG_slab (1u << 2)

struct kmem_cache;

struct page {
    uint16_t flags;
    uint16_t order;
    int32_t refcount;
    /* buddy: the free list of its order, slab: the partial list of its cache */
    struct list_head lru;
    /* PG_slab only, a slab is exactly one page so these describe the whole slab */
    struct kmem_cache *slab_cache;
    void *freelist;                 /* first free object, NULL when the slab is full */
    uint32_t inuse;                 /* objects handed out, 0 is empty, cache->objects is full */
};
#define page_free(page) 

extern struct page *mem_map;
#define pfn_to_page(pfn) (mem_map + (pfn))
#define page_to_pfn(page) ((uint32_t)((page) - mem_map))

#define page_to_phys(page) (page_to_pfn(page) << PAGE_SHIFT)
#define page_address(page) phys_to_virt(page_to_phys(page))
/* Clearly for the top 1G address only (those that get mapped) */
#define virt_to_page(va) pfn_to_page(virt_to_phys(va) >> PAGE_SHIFT)

/* Number of free blocks, not pages */
struct free_block {
    struct list_head free_list;
    uint32_t nr_free;
};

#define MAX_ORDER 10

/* Order = pow(2, order) number of pages */
struct zone {
    struct free_block free_blocks[MAX_ORDER + 1];
    uint32_t free_pages;
};

/* allocate mem_map and give every free frame (usable minus reserved) to the buddy system, after init_pd */
void init_page_alloc(void);
/* 2^order contiguous pages, NULL when no block that large is free */
struct page *alloc_pages(uint32_t order);
/* order must be the one alloc_pages was called with */
void free_pages(struct page *page, uint32_t order);

#endif
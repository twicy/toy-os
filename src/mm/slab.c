#include <mm/slab.h>
#include <mm/page_alloc.h>
#include <mm/memblock.h>
#include <debug.h>
#include <math.h>
#include <stddef.h>

void kmem_cache_init(struct kmem_cache *cache, const char *name, uint32_t size, uint32_t align) {
    if (align == 0)
        align = sizeof(void *);
    ASSERT(is_pow2(align));
    ASSERT(size <= PAGE_SIZE && align <= PAGE_SIZE);

    size = ALIGN_UP(MAX(size, sizeof(void *)), align);

    cache->name = name;
    cache->size = size;
    cache->objects = PAGE_SIZE / size;
    INIT_LIST_HEAD(&cache->partial);
}

/* Carve a fresh page */
static struct page *new_slab(struct kmem_cache *cache) {
    struct page *page = alloc_pages(0);
    char *base;

    if (!page)
        return NULL;

    base = page_address(page);
    page->freelist = NULL;
    /* Each slot either holds a valid obj or pointer to the next slot */
    /* As long as the slab is "partial", it's freelist is bound to point to a
     * free slot which should further point to its next free slot if possible
     */
    for (uint32_t i = cache->objects; i > 0; i--) {
        void **obj = (void **)(base + (i - 1) * cache->size);
        *obj = page->freelist;
        page->freelist = obj;
    }
    page->flags |= PG_slab;
    page->slab_cache = cache;
    page->inuse = 0;
    list_add(&page->lru, &cache->partial);
    return page;
}

/* free_pages = return to buddy */
static void free_slab(struct page *page) {
    list_del_init(&page->lru);
    page->flags &= ~PG_slab;
    page->slab_cache = NULL;
    page->freelist = NULL;
    free_pages(page, 0);
}

void *kmem_cache_alloc(struct kmem_cache *cache) {
    struct page *page;
    void **obj;

    if (list_empty(&cache->partial) && !new_slab(cache))
        return NULL;

    /* pop the head of the first partial slab's free list */
    page = list_first_entry(&cache->partial, struct page, lru);
    obj = page->freelist;
    page->freelist = *obj;
    page->inuse++;

    /* a full slab leaves the partial list, kmem_cache_free puts it back */
    if (page->inuse == cache->objects)
        /* Do not worry about losing track of full slabs, see kmem_cache_free() */
        list_del_init(&page->lru);
    return obj;
}

void kmem_cache_free(struct kmem_cache *cache, void *obj) {
    struct page *page;

    /* only the direct map has a struct page behind it: an address below
     * KERNEL_OFFSET wraps around to a huge pfn, one past RAM is >= max_pfn */
    ASSERT(PFN_DOWN(virt_to_phys(obj)) < max_pfn);
    page = virt_to_page(obj);
    ASSERT((page->flags & PG_slab) && page->slab_cache == cache);
    /* must be the start of an object, not a pointer into the middle of one */
    ASSERT(((uintptr_t)obj - (uintptr_t)page_address(page)) % cache->size == 0);
    /* a cheap double free check, it only catches freeing the last freed object again */
    ASSERT(page->inuse > 0 && obj != page->freelist);

    /* a full slab is about to have a free object, so it can serve allocations again */
    if (page->inuse == cache->objects)
        list_add(&page->lru, &cache->partial);

    /* push onto the free list */
    *(void **)obj = page->freelist;
    page->freelist = obj;
    page->inuse--;

    /* keep one slab even when empty, so allocating and freeing a single
     * object in a loop does not go to the buddy system every time */
    if (page->inuse == 0 && !list_is_singular(&cache->partial))
        free_slab(page);
}

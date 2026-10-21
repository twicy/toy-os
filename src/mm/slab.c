#include <mm/slab.h>
#include <mm/page_alloc.h>
#include <mm/memblock.h>
#include <debug.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

#define KMALLOC_CACHES (KMALLOC_MAX_SHIFT - KMALLOC_MIN_SHIFT + 1)

static struct kmem_cache kmalloc_caches[KMALLOC_CACHES];

static const char *const kmalloc_names[KMALLOC_CACHES] = {
    "kmalloc-8", "kmalloc-16", "kmalloc-32", "kmalloc-64", "kmalloc-128",
    "kmalloc-256", "kmalloc-512", "kmalloc-1024", "kmalloc-2048",
};

/* index of the smallest kmalloc cache holding size bytes, 0 < size <= KMALLOC_MAX_CACHE_SIZE */
static inline uint32_t kmalloc_index(uint32_t size) {
    if (size <= (1u << KMALLOC_MIN_SHIFT))
        return 0;
    /* 32 - clz(size - 1) is log2 of size rounded up to a power of 2 */
    return 32 - __builtin_clz(size - 1) - KMALLOC_MIN_SHIFT;
}

/* the struct page behind a direct mapped address, anything else is a caller bug */
static struct page *obj_to_page(const void *obj) {
    /* an address below KERNEL_OFFSET wraps around to a huge pfn, one past RAM is >= max_pfn */
    ASSERT(PFN_DOWN(virt_to_phys(obj)) < max_pfn);
    return virt_to_page(obj);
}

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
    struct page *page = obj_to_page(obj);

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

void init_kmalloc(void) {
    for (uint32_t i = 0; i < KMALLOC_CACHES; i++)
        kmem_cache_init(&kmalloc_caches[i], kmalloc_names[i], 1u << (i + KMALLOC_MIN_SHIFT), 0);
}

struct kmem_cache *kmem_cache_create(const char *name, uint32_t size, uint32_t align) {
    struct kmem_cache *cache = kmalloc(sizeof(*cache));

    if (cache)
        kmem_cache_init(cache, name, size, align);
    return cache;
}

void *kmalloc(size_t size) {
    struct page *page;
    uint32_t order;

    if (size == 0 || size > KMALLOC_MAX_SIZE)
        return NULL;
    if (size <= KMALLOC_MAX_CACHE_SIZE)
        return kmem_cache_alloc(&kmalloc_caches[kmalloc_index(size)]);

    /* too large for a slab: a buddy block of its own, its first page
     * remembers the order so kfree knows how much to give back */
    order = get_order(size);
    page = alloc_pages(order);
    if (!page)
        return NULL;
    page->flags |= PG_large;
    page->order = order;
    return page_address(page);
}

void *kzalloc(size_t size) {
    void *ptr = kmalloc(size);

    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}

void kfree(const void *ptr) {
    struct page *page;
    uint32_t order;

    if (!ptr)
        return;

    /* the page says which kind of kmalloc it was */
    page = obj_to_page(ptr);
    if (page->flags & PG_slab) {
        kmem_cache_free(page->slab_cache, (void *)ptr);
        return;
    }

    ASSERT((page->flags & PG_large) && ptr == page_address(page));
    order = page->order;
    page->flags &= ~PG_large;
    page->order = 0;
    free_pages(page, order);
}

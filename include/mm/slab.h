#ifndef MM_SLAB_H
#define MM_SLAB_H
#include <stdint.h>
#include <stddef.h>
#include <list.h>
#include <kernel.h>
#include <mm/page_alloc.h>

/* Example: struct kmem_cache "kmalloc-1024"
 * size = 1024, objects = 4
 * partial is a linked list whose container is struct page
 */
struct kmem_cache {
    const char *name;
    uint32_t size;
    uint32_t objects;
    /* Non-full slabs, slab = page frame here */
    struct list_head partial;
};

void kmem_cache_init(struct kmem_cache *cache, const char *name, uint32_t size, uint32_t align);
void *kmem_cache_alloc(struct kmem_cache *cache);
void kmem_cache_free(struct kmem_cache *cache, void *obj);

/* kmalloc of 8, 16, ..., 2048 bytes comes from a slab of one of the kmalloc
 * caches, anything larger is a buddy block of its own, up to the largest one */
#define KMALLOC_MIN_SHIFT 3
#define KMALLOC_MAX_SHIFT 11
#define KMALLOC_MAX_CACHE_SIZE (1u << KMALLOC_MAX_SHIFT)
#define KMALLOC_MAX_SIZE ((uint32_t)PAGE_SIZE << MAX_ORDER)

/* set up the kmalloc caches, after init_page_alloc */
void init_kmalloc(void);
/* a cache in kmalloc'ed storage, after init_kmalloc */
struct kmem_cache *kmem_cache_create(const char *name, uint32_t size, uint32_t align);

/* Physically contiguous and in the direct map. Aligned to size rounded up to
 * a power of 2, up to PAGE_SIZE. NULL when out of memory or size is 0 */
void *kmalloc(size_t size);
void *kzalloc(size_t size);
/* NULL is ignored */
void kfree(const void *ptr);

#endif

#ifndef MM_SLAB_H
#define MM_SLAB_H
#include <stdint.h>
#include <list.h>

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

#endif

#ifndef MM_VMALLOC_H
#define MM_VMALLOC_H
#include <stddef.h>

/* install the page tables of the whole vmalloc space, after init_kmalloc */
void init_vmalloc(void);

/* Page aligned and virtually contiguous in [VMALLOC_START, VMALLOC_END),
 * backed page by page, so the frames are scattered and there is no
 * phys_to_virt / virt_to_page for it. An unmapped guard page follows every
 * area. NULL when out of memory or vmalloc space, or size is 0 */
void *vmalloc(size_t size);
/* NULL is ignored */
void vfree(const void *addr);

#endif

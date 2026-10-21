#include <mm/vmalloc.h>
#include <mm/slab.h>
#include <mm/page_alloc.h>
#include <mm/paging.h>
#include <list.h>
#include <debug.h>
#include <math.h>
#include <string.h>

/* one vmalloc'ed range, the guard page after it is not counted in size */
struct vm_area {
    uint32_t addr;
    uint32_t size;
    struct list_head list;
};

/* every live area, ascending by addr */
static struct list_head vm_areas;

/* init_vmalloc installed every vmalloc page table, so there always is one */
static pte_t *vmalloc_pte(uint32_t va) {
    pte_t *pt = phys_to_virt(pde_phys(kernel_pd[PD_INDEX(va)]));
    return &pt[PT_INDEX(va)];
}

void init_vmalloc(void) {
    /* The page tables are never freed, so no PDE of the kernel half changes
     * after this: a process page directory can copy PDEs 768-1023 from
     * kernel_pd once, and still see every later vmalloc mapping, since
     * those land in the shared page tables, not in the PDEs */
    for (uint32_t va = VMALLOC_START; va < VMALLOC_END; va += PGDIR_SIZE) {
        struct page *page = alloc_pages(0);

        if (!page)
            PANIC("no memory for the vmalloc page tables");
        memset(page_address(page), 0, PAGE_SIZE);
        kernel_pd[PD_INDEX(va)] = mk_pde(page_to_phys(page), PG_KERNEL);
    }
    INIT_LIST_HEAD(&vm_areas);
}

static struct vm_area *alloc_vm_area(uint32_t size) {
    struct vm_area *area = kmalloc(sizeof(*area));
    struct list_head *pos;
    uint32_t addr = VMALLOC_START;

    if (!area)
        return NULL;

    list_for_each(pos, &vm_areas) {
        struct vm_area *next = list_entry(pos, struct vm_area, list);

        if (next->addr - addr >= size + PAGE_SIZE)
            break;
        addr = next->addr + next->size + PAGE_SIZE;
    }

    if (VMALLOC_END - addr < size + PAGE_SIZE) {
        kfree(area);
        return NULL;
    }
    area->addr = addr;
    area->size = size;
    list_add_tail(&area->list, pos);
    return area;
}

static void free_vm_area(struct vm_area *area) {
    list_del(&area->list);
    kfree(area);
}

static void unmap_pages(uint32_t addr, uint32_t size) {
    for (uint32_t va = addr; va < addr + size; va += PAGE_SIZE) {
        pte_t *pte = vmalloc_pte(va);
        struct page *page = pfn_to_page(PFN_DOWN(pte_phys(*pte)));

        *pte = 0;
        invlpg(va);
        free_pages(page, 0);
    }
}

void *vmalloc(size_t size) {
    struct vm_area *area;

    if (size == 0 || size > VMALLOC_END - VMALLOC_START)
        return NULL;

    area = alloc_vm_area(ALIGN_UP(size, PAGE_SIZE));
    if (!area)
        return NULL;

    for (uint32_t off = 0; off < area->size; off += PAGE_SIZE) {
        struct page *page = alloc_pages(0);

        if (!page) {
            unmap_pages(area->addr, off);
            free_vm_area(area);
            return NULL;
        }

        *vmalloc_pte(area->addr + off) = mk_pte(page_to_phys(page), PG_KERNEL);
    }
    return (void *)area->addr;
}

void vfree(const void *addr) {
    struct vm_area *area = NULL;
    struct list_head *pos;

    if (!addr)
        return;

    list_for_each(pos, &vm_areas) {
        struct vm_area *a = list_entry(pos, struct vm_area, list);

        if (a->addr == (uint32_t)addr) {
            area = a;
            break;
        }
    }
    if (!area)
        PANIC("vfree of %p, which vmalloc did not return", addr);

    unmap_pages(area->addr, area->size);
    free_vm_area(area);
}

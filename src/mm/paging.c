#include <mm/paging.h>
#include <mm/memblock.h>
#include <debug.h>
#include <stddef.h>
#include <string.h>
#include <math.h>

/* kernel virtual addresses */
pde_t *kernel_pd;
pte_t *kernel_pts;

/* A brief recap of loader pt mapping:
 * PD starts at 0x100000, PTs start at 0x101000
 * PDE 0 and both PDE 768 point to PT0, which covers the first 1 MB
 * PDE 769 - 1022 are mapped to PT 769 - 1022, but PTEs are empty
 * PDE 1023 points to PD
 */
#define LOADER_PTES ((pte_t *)0xFFC00000)

/* Mapping kernel_pd and kernel_pts virtual addr, for later arr + index access */
static void *early_alloc(uint32_t size) {
    uint32_t phys = phys_mem_alloc(size, PAGE_SIZE);

    if (phys == 0) {
        PANIC("no physical memory for %u bytes of page tables", size);
    }
    for (uint32_t off = 0; off < size; off += PAGE_SIZE) {
        uint32_t va = (uint32_t)phys_to_virt(phys + off);
        /* All PTs (arry of PTEs) are packed */
        LOADER_PTES[va >> PAGE_SHIFT] = mk_pte(phys + off, PG_KERNEL);
        invlpg(va);
    }
    return phys_to_virt(phys);
}

void init_pd(void) {
    uint32_t n_pts = DIV_ROUND_UP(max_pfn, PT_ENTRIES);
    uint32_t first_pde = PD_INDEX(KERNEL_OFFSET);

    /* memblock already clipped RAM at PHYS_ADDR_LIMIT, this only catches the two disagreeing */
    if (first_pde + n_pts > PD_INDEX(VMALLOC_START)) {
        PANIC("%u MiB of RAM runs into vmalloc space", max_pfn >> (20 - PAGE_SHIFT));
    }

    /* Create a new kernel page dir, not colliding with loader's */
    kernel_pd = early_alloc(PAGE_SIZE);
    kernel_pts = early_alloc(n_pts * PAGE_SIZE);
    memset(kernel_pd, 0, PAGE_SIZE);
    memset(kernel_pts, 0, n_pts * PAGE_SIZE);

    for (uint32_t pfn = 0; pfn < max_pfn; pfn++) {
        kernel_pts[pfn] = mk_pte(pfn << PAGE_SHIFT, PG_KERNEL);
    }
    for (uint32_t i = 0; i < n_pts; i++) {
        kernel_pd[first_pde + i] = mk_pde(virt_to_phys(&kernel_pts[i * PT_ENTRIES]), PG_KERNEL);
    }

    /* no identity map of low memory, so virtual 0 faults, and no recursive entry:
     * page tables are reached through the direct map from here on */
    asm volatile ("mov %0, %%cr3" : : "r"(virt_to_phys(kernel_pd)) : "memory");

    /* the CPU no longer walks the loader's page directory and page tables, give them back */
    phys_mem_free(LOADER_PAGE_TABLES_START, LOADER_PAGE_TABLES_END);
}

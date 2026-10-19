#ifndef MM_PAGING_H
#define MM_PAGING_H
#include <stdint.h>
#include <kernel.h>

#define PD_ENTRIES 1024
#define PT_ENTRIES 1024
#define PGDIR_SHIFT 22
/* Every PGD covers 4MB of memory */
#define PGDIR_SIZE (1u << PGDIR_SHIFT)

#define PD_INDEX(va) (((uint32_t)(va) >> PGDIR_SHIFT) & (PD_ENTRIES - 1))
#define PT_INDEX(va) (((uint32_t)(va) >> PAGE_SHIFT) & (PT_ENTRIES - 1))

typedef uint32_t pde_t;
typedef uint32_t pte_t;

/* in every PDE and PTE */
#define PG_P (1u << 0)      /* present, the CPU ignores all other bits when clear */
#define PG_RW (1u << 1)     /* writable, read-only when clear */
#define PG_US (1u << 2)     /* user mode may access, kernel only when clear */
#define PG_PWT (1u << 3)    /* write-through caching */
#define PG_PCD (1u << 4)    /* caching disabled */
#define PG_A (1u << 5)      /* accessed, set by the CPU */

/* in a PTE, and in a PDE with PDE_PS set, ignored in a PDE pointing to a page table */
#define PG_D (1u << 6)      /* dirty, set by the CPU on a write */
#define PG_G (1u << 8)      /* global, kept in the TLB across cr3 loads when CR4.PGE is set */

/* bit 7 means different things */
#define PDE_PS (1u << 7)    /* PDE: maps a 4 MiB page instead of pointing to a page table, needs CR4.PSE */
#define PTE_PAT (1u << 7)   /* PTE: page attribute table index */
#define PDE_PS_PAT (1u << 12) /* PDE with PDE_PS: PAT moves here, the address starts at bit 22 */

/* bits 9..11 are free for the OS in both */
#define PG_AVL_MASK (7u << 9)

/* physical address field */
#define PG_FRAME_MASK 0xfffff000u       /* PTE, PDE pointing to a page table: 4 KiB aligned */
#define PDE_PS_FRAME_MASK 0xffc00000u   /* PDE with PDE_PS: 4 MiB aligned */

#define PG_KERNEL (PG_P | PG_RW)
#define PG_USER (PG_P | PG_RW | PG_US)

/* build entries */
#define mk_pte(phys, flags) ((pte_t)(((uint32_t)(phys) & PG_FRAME_MASK) | (flags)))
#define mk_pde(pt_phys, flags) ((pde_t)(((uint32_t)(pt_phys) & PG_FRAME_MASK) | (flags)))
#define mk_pde_4m(phys, flags) ((pde_t)(((uint32_t)(phys) & PDE_PS_FRAME_MASK) | PDE_PS | (flags)))

/* physical address an entry points to: a page, a page table, or (PDE_PS) a 4 MiB page */
#define pte_phys(e) ((uint32_t)(e) & PG_FRAME_MASK)
#define pde_phys(e) ((uint32_t)(e) & (((e) & PDE_PS) ? PDE_PS_FRAME_MASK : PG_FRAME_MASK))

extern pde_t *kernel_pd;
extern pte_t *kernel_pts;

/* Re-create PD and PTs for the kernel */
void init_pd(void);

#endif

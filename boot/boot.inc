LOADER_BASE_ADDR equ 0x900
LOADER_START_SECTOR equ 0x2

; KERNEL_START_SECTOR and KERNEL_SECTOR_COUNT must match seek and count in the Makefile
KERNEL_START_SECTOR equ 0x9
KERNEL_SECTOR_COUNT equ 200
; kernel.bin (ELF file) is read here first, 200 sectors end at 0x89000
KERNEL_BIN_BASE_ADDR equ 0x70000
; top of free low memory, below the EBDA at 0x9fc00
KERNEL_STACK_TOP equ 0xc009f000

; Segment Descriptor
; Higher 32 bits
DESC_BASE_31_24_OFFSET equ 24
DESC_G_OFFSET equ 23
DESC_DB_OFFSET equ 22
DESC_L_OFFSET equ 21
DESC_AVL_OFFSET equ 20
DESC_LIMIT_19_16_OFFSET equ 16
DESC_P_OFFSET equ 15
DESC_DPL_OFFSET equ 13
DESC_S_OFFSET equ 12
DESC_TYPE_OFFSET equ 8
DESC_BASE_23_16_OFFSET equ 0

; Lower 32 bits
DESC_BASE_15_0_OFFSET equ 16
DESC_LIMIT_15_0_OFFSET equ 0

; Code segment
; Base: 0, Limit: 0xFFFFF * 4K = 4G
; Type: x=1,c=0,r=1,a=0
DESC_CODE_HIGH_32 equ (0 << DESC_BASE_31_24_OFFSET) + \
                      (1 << DESC_G_OFFSET) + \
                      (1 << DESC_DB_OFFSET) + \
                      (0 << DESC_L_OFFSET) + \
                      (0 << DESC_AVL_OFFSET) + \
                      (1111b << DESC_LIMIT_19_16_OFFSET) + \
                      (1 << DESC_P_OFFSET) + \
                      (00b << DESC_DPL_OFFSET) + \
                      (1 << DESC_S_OFFSET) + \
                      (1010b << DESC_TYPE_OFFSET) + \
                      (0 << DESC_BASE_23_16_OFFSET)

DESC_CODE_LOW_32 equ (0 << DESC_BASE_15_0_OFFSET) + \
                     (0xFFFF << DESC_LIMIT_15_0_OFFSET)

; Data segment
; Base: 0, Limit: 0xFFFFF * 4K = 4G
; Type: x=0,e=0,w=1,a=0
DESC_DATA_HIGH_32 equ (0 << DESC_BASE_31_24_OFFSET) + \
                      (1 << DESC_G_OFFSET) + \
                      (1 << DESC_DB_OFFSET) + \
                      (0 << DESC_L_OFFSET) + \
                      (0 << DESC_AVL_OFFSET) + \
                      (1111b << DESC_LIMIT_19_16_OFFSET) + \
                      (1 << DESC_P_OFFSET) + \
                      (00b << DESC_DPL_OFFSET) + \
                      (1 << DESC_S_OFFSET) + \
                      (0010b << DESC_TYPE_OFFSET) + \
                      (0 << DESC_BASE_23_16_OFFSET)

DESC_DATA_LOW_32 equ (0 << DESC_BASE_15_0_OFFSET) + \
                     (0xFFFF << DESC_LIMIT_15_0_OFFSET)

; Video segment
; Base: 0xB8000, Limit: 8 * 4K = 32K
DESC_VIDEO_HIGH_32 equ (0 << DESC_BASE_31_24_OFFSET) + \
                      (1 << DESC_G_OFFSET) + \
                      (1 << DESC_DB_OFFSET) + \
                      (0 << DESC_L_OFFSET) + \
                      (0 << DESC_AVL_OFFSET) + \
                      (0000b << DESC_LIMIT_19_16_OFFSET) + \
                      (1 << DESC_P_OFFSET) + \
                      (00b << DESC_DPL_OFFSET) + \
                      (1 << DESC_S_OFFSET) + \
                      (0010b << DESC_TYPE_OFFSET) + \
                      (0xB << DESC_BASE_23_16_OFFSET)

DESC_VIDEO_LOW_32 equ (0x8000 << DESC_BASE_15_0_OFFSET) + \
                     (0x0007 << DESC_LIMIT_15_0_OFFSET)

; Segment selector
SELECTOR_DESC_IDX_OFFSET equ 3
SELECTOR_TI_OFFSET equ 2
SELECTOR_RPL_OFFSET equ 0

SELECTOR_CODE equ (1 << SELECTOR_DESC_IDX_OFFSET) + \
                  (0b << SELECTOR_TI_OFFSET) + \
                  (00b << SELECTOR_RPL_OFFSET)
SELECTOR_DATA equ (2 << SELECTOR_DESC_IDX_OFFSET) + \
                  (0b << SELECTOR_TI_OFFSET) + \
                  (00b << SELECTOR_RPL_OFFSET)
SELECTOR_VIDEO equ (3 << SELECTOR_DESC_IDX_OFFSET) + \
                  (0b << SELECTOR_TI_OFFSET) + \
                  (00b << SELECTOR_RPL_OFFSET)

; Paging
PAGE_OFFSET equ 12
PAGE_DIR_BASE equ 0x100000
PAGE_TABLE_0_BASE equ 0x101000
PAGE_0_BASE equ 0x0

PDE_ADDR_31_12_OFFSET equ 12
PDE_AVL_OFFSET equ 8
PDE_G_OFFSET equ 8
PDE_ZERO_OFFSET equ 7
PDE_D_OFFSET equ 6
PDE_A_OFFSET equ 5
PDE_PCD_OFFSET equ 4
PDE_PWT_OFFSET equ 3
PDE_US_OFFSET equ 2
PDE_RW_OFFSET equ 1
PDE_P_OFFSET equ 0

PTE_ADDR_31_12_OFFSET equ 12
PTE_AVL_OFFSET equ 9
PTE_G_OFFSET equ 8
PTE_PAT_OFFSET equ 7
PTE_D_OFFSET equ 6
PTE_A_OFFSET equ 5
PTE_PCD_OFFSET equ 4
PTE_PWT_OFFSET equ 3
PTE_US_OFFSET equ 2
PTE_RW_OFFSET equ 1
PTE_P_OFFSET equ 0

PDE_FLAGS equ (0 << PDE_AVL_OFFSET) + \
              (0 << PDE_G_OFFSET) + \
              (0 << PDE_ZERO_OFFSET) + \
              (0 << PDE_D_OFFSET) + \
              (0 << PDE_A_OFFSET) + \
              (0 << PDE_PCD_OFFSET) + \
              (0 << PDE_PWT_OFFSET) + \
              (1 << PDE_US_OFFSET) + \
              (1 << PDE_RW_OFFSET) + \
              (1 << PDE_P_OFFSET)

PTE_FLAGS equ (0 << PTE_AVL_OFFSET) + \
              (0 << PTE_G_OFFSET) + \
              (0 << PTE_PAT_OFFSET) + \
              (0 << PTE_D_OFFSET) + \
              (0 << PTE_A_OFFSET) + \
              (0 << PTE_PCD_OFFSET) + \
              (0 << PTE_PWT_OFFSET) + \
              (1 << PTE_US_OFFSET) + \
              (1 << PTE_RW_OFFSET) + \
              (1 << PTE_P_OFFSET)

PDE_0 equ PAGE_TABLE_0_BASE + PDE_FLAGS
PTE_0 equ PAGE_0_BASE + PTE_FLAGS

; ELF
PT_NULL equ 0
PT_LOAD equ 1
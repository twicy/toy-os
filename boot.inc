LOADER_BASE_ADDR equ 0x900
LOADER_START_SECTOR equ 0x2

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
%include "boot.inc"
section loader vstart=LOADER_BASE_ADDR
jmp loader_entry

; data section
GDT_BASE:
RESV_DESC:
    dd 0x0
    dd 0x0
CODE_DESC:
    dd DESC_CODE_LOW_32
    dd DESC_CODE_HIGH_32
DATA_DESC:
    dd DESC_DATA_LOW_32
    dd DESC_DATA_HIGH_32
VIDEO_DESC:
    dd DESC_VIDEO_LOW_32
    dd DESC_VIDEO_HIGH_32
times 60 dq 0
GDT_SIZE equ $ - GDT_BASE
GDT_LIMIT equ GDT_SIZE - 1

; lgdt 48-bit
GDT_PTR:
    dw GDT_LIMIT
    dd GDT_BASE

; code section
loader_entry:
    ; disable interrupts - no IDT yet, any interrupt in prot mode would triple fault
    cli

    ; enable A20
    in al, 0x92
    or al, 0000_0010b
    out 0x92, al

    ; load GDT
    lgdt [GDT_PTR]

    ; open protection mode - set cr0 bit 0
    mov eax, cr0
    or eax, 0x00000001
    mov cr0, eax

    ; refresh pipeline
    jmp dword SELECTOR_CODE:prot_mode_entry

[bits 32]
prot_mode_entry:
    ; segment registers still hold real-mode values, reload with selectors
    mov ax, SELECTOR_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax
    ; stack grows down from just below the loader
    mov esp, LOADER_BASE_ADDR

    mov ax, SELECTOR_VIDEO
    mov gs, ax

    ; row 1, col 0: each row is 80 chars * 2 bytes = 160
    mov byte [gs:160], 'P'
    mov byte [gs:161], 0xA4

setup_gpt:
    xor eax, eax
PURGE_LEN equ (1024 * 4 + 1024 * 1024 * 4)
.purge_mem:
    mov byte [PAGE_DIR_BASE + eax], 0
    inc eax
    cmp eax, PURGE_LEN
    jl .purge_mem

    ; Setting up PDE0 and PT0
    mov dword [PAGE_DIR_BASE], PDE_0
    mov dword [PAGE_DIR_BASE + 768 * 4], PDE_0

    xor eax, eax
    ; PTE for frame 0, advanced one frame per entry
    mov ebx, PTE_0

; Map the first 1M of memory
; VA [0x0, 0x100000) and VA[0xc0000000, 0xc0100000) map to PA [0x0, 0x100000)
.setup_pt0:
    mov dword [PAGE_TABLE_0_BASE + 4 * eax], ebx
    add ebx, 4096
    inc eax
    cmp eax, 256
    jl .setup_pt0

; Recursive Mapping of Page Directory
; The key here is PTE and PDE have almost the same field, so the PD here also plays as a PT
; e.g. VA 0xFFC00001 maps to 1023 th PDE entry (by taking the first 10 bits)
; Then the PA of the PT is resolved as 0x100000, the PD itself (what gets filled in here)
; The next 10 bits point to 0 th entry, which points to 0x101000 (PT0, PDE_0 filled in above)
; plus offset 1, the final address is 0x101001, i.e. byte 1 of PT0
    mov dword [PAGE_DIR_BASE + 1023 * 4], PAGE_DIR_BASE + PDE_FLAGS

; Map other kernel PDEs
    mov ecx, 769
    mov ebx, PAGE_TABLE_0_BASE + 769 * 4096 + PDE_FLAGS
.setup_kernel_pdes:
    mov dword [PAGE_DIR_BASE + ecx * 4], ebx
    add ebx, 4096
    inc ecx
    cmp ecx, 1023
    jl .setup_kernel_pdes

; Move GDT, video segment and stack to their kernel aliases
    sgdt [GDT_PTR]
    ; video desc base 31..24 is the top byte of its high dword: 0xB8000 -> 0xC00B8000
    or dword [VIDEO_DESC + 4], 0xC0000000
    ; GDTR base 0x903 -> 0xC0000903
    add dword [GDT_PTR + 2], 0xC0000000
    add esp, 0xC0000000

; Enable paging
    ; Bochs magic breakpoint
    xchg bx, bx

    mov eax, PAGE_DIR_BASE
    mov cr3, eax
    ; set cr0 bit 31 (PG)
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    ; GDTR base is a linear address, load the high one only after paging is on
    lgdt [GDT_PTR]
    ; gs still caches the old video base until the selector is reloaded
    mov ax, SELECTOR_VIDEO
    mov gs, ax

    ; row 1, col 1: printed through the new 0xC00B8000 video base
    mov byte [gs:162], 'V'
    mov byte [gs:163], 0xA4

    mov byte [0xC00B8000 + 164], 'K'
    mov byte [0xC00B8000 + 165], 0xA5

    jmp $
    
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

    jmp $
    
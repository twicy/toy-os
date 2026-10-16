%include "boot.inc"

section loader vstart=LOADER_BASE_ADDR
jmp loader_entry

; temporary gdt to enter protection mode
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
GDT_SIZE equ $ - GDT_BASE
GDT_LIMIT equ GDT_SIZE - 1

; lgdt 48-bit
GDT_PTR:
    dw GDT_LIMIT
    dd GDT_BASE

mem_error_msg:
    db "E820 memory detection failed",13,10
mem_error_msg_len equ ($ - mem_error_msg)

prot_success_msg:
    db "Successfully entered protection mode",13,10
prot_success_msg_len equ ($ - prot_success_msg)

paging_success_msg:
    db "Successfully enabled paging",13,10
paging_success_msg_len equ ($ - paging_success_msg)

loader_entry:
    ; BIOS calls only work in real mode, collect the memory map first
    call detect_memory

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

; Store the BIOS E820 memory map at MEM_MAP_ADDR (layout in boot.inc)
; ds = es = 0 from the MBR
; prints and halts if the BIOS returns no entries
detect_memory:
    mov dword [MEM_MAP_COUNT_ADDR], 0
    ; es:di = where the BIOS writes the next entry
    mov di, MEM_MAP_ENTRIES_ADDR
    ; continuation value, 0 = start from the first entry
    xor ebx, ebx
.next_entry:
    mov eax, 0xe820
    ; 'SMAP', set on every call since some BIOSes trash edx
    mov edx, 0x534d4150
    mov ecx, MEM_MAP_ENTRY_SIZE
    int 0x15
    ; cf set: no E820 on the first call, end of list on later ones
    jc .done
    ; eax = 'SMAP' confirms the BIOS understood the call
    cmp eax, 0x534d4150
    jne .done
    ; skip entries with a 0 qword length at offset 8
    mov eax, [di + 8]
    or eax, [di + 12]
    jz .skip
    inc dword [MEM_MAP_COUNT_ADDR]
    add di, MEM_MAP_ENTRY_SIZE
    ; stop before the map grows into the loader's stack
    cmp dword [MEM_MAP_COUNT_ADDR], MEM_MAP_MAX_ENTRIES
    jae .done
.skip:
    ; ebx = 0: that was the last entry
    test ebx, ebx
    jnz .next_entry
.done:
    cmp dword [MEM_MAP_COUNT_ADDR], 0
    je .failed
    ret
.failed:
    mov bp, mem_error_msg
    mov cx, mem_error_msg_len
    call print_real_mode
    jmp halt_real_mode

; print_real_mode, halt_real_mode
%include "util.inc"

[bits 32]
prot_mode_entry:
    ; segment registers still hold real-mode values, reload with selectors
    mov ax, SELECTOR_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, LOADER_STACK_TOP
    mov ax, SELECTOR_VIDEO
    mov gs, ax

    push prot_success_msg
    push prot_success_msg_len
    call print_prot_mode
    add esp, 8

    ; load kernel.bin into a buffer, it is an ELF file so its code is not at offset 0
    mov eax, KERNEL_START_SECTOR
    mov ebx, KERNEL_BIN_BASE_ADDR
    mov ecx, KERNEL_SECTOR_COUNT
    call read_disk

    call setup_gpt

; Move GDT, video segment and stack to their kernel aliases
    sgdt [GDT_PTR]
    ; video desc base 31..24 is the top byte of its high dword: 0xB8000 -> 0xC00B8000
    or dword [VIDEO_DESC + 4], 0xC0000000
    ; GDTR base 0x803 -> 0xC0000803
    add dword [GDT_PTR + 2], 0xC0000000
    add esp, 0xC0000000

; Enable paging
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

    ; printed through the new 0xC00B8000 video base
    push paging_success_msg
    push paging_success_msg_len
    call print_prot_mode
    add esp, 8

    ; Bochs magic breakpoint
    xchg bx, bx

    call kernel_init

    mov esp, KERNEL_STACK_TOP
    ; e_entry
    jmp [KERNEL_BIN_BASE_ADDR + 24]

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
    ret

kernel_init:
    xor eax, eax
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx

    ; e_phentsize, namely program header size
    mov dx, [KERNEL_BIN_BASE_ADDR + 42]
    ; e_phoff, offset of the first program header
    mov ebx, [KERNEL_BIN_BASE_ADDR + 28]
    ; e_phnum, number of program header
    mov cx, [KERNEL_BIN_BASE_ADDR + 44]

    add ebx, KERNEL_BIN_BASE_ADDR

.each_segment:
    ; p_type is a dword, only PT_LOAD segments get copied into memory
    cmp dword [ebx], PT_LOAD
    jne .next_segment

    ; memcpy from bin to the designated place
    ; p_filesz
    push dword [ebx + 16]
    ; p_offset
    mov eax, [ebx + 4]
    add eax, KERNEL_BIN_BASE_ADDR
    push eax
    ; p_vaddr
    push dword [ebx + 8]
    call mem_cpy
    add esp, 12

    ; .bss takes memory but no file bytes (p_memsz > p_filesz), zero
    ; the tail so C globals start at 0
    ; p_memsz - p_filesz
    mov eax, [ebx + 20]
    sub eax, [ebx + 16]
    push eax
    ; p_vaddr + p_filesz
    mov eax, [ebx + 8]
    add eax, [ebx + 16]
    push eax
    call mem_zero
    add esp, 8

.next_segment:
    add ebx, edx
    loop .each_segment
    ret

read_disk:
    mov esi, eax
    mov edi, ecx

    mov dx, 0x1f2
    mov al, cl
    out dx, al

    mov eax, esi

    mov dx, 0x1f3
    out dx, al

    mov cl, 8
    shr eax, cl
    mov dx, 0x1f4
    out dx, al

    shr eax, cl
    mov dx, 0x1f5
    out dx, al

    shr eax, cl
    and al, 0x0f
    or al, 0xe0
    mov dx, 0x1f6
    out dx, al

    mov dx, 0x1f7
    mov al, 0x20
    out dx, al

.next_sector:
    mov dx, 0x1f7
.not_ready:
    nop
    in al, dx
    and al, 0x88
    cmp al, 0x08
    jnz .not_ready

    mov ecx, 256
    mov dx, 0x1f0
.go_on_read:
    in ax, dx
    mov [ebx], ax
    add ebx, 2
    loop .go_on_read

    dec edi
    jnz .next_sector
    ret

; dst, src, size
mem_cpy:
    push ebp
    mov ebp, esp
    push ecx
    mov edi, [ebp + 8]
    mov esi, [ebp + 12]
    mov ecx, [ebp + 16]
    ; movsb only copies forward when the direction flag is clear
    cld
    rep movsb

    pop ecx
    pop ebp
    ret

; dst, size
; clobbers eax, edi
mem_zero:
    push ebp
    mov ebp, esp
    push ecx
    mov edi, [ebp + 8]
    mov ecx, [ebp + 12]
    xor eax, eax
    ; stosb stores al and only moves forward when the direction flag is clear
    cld
    rep stosb

    pop ecx
    pop ebp
    ret

; clobbers eax = the function overwrites eax and doesn't restore it
print_prot_mode:
    push ebp
    mov ebp, esp
    push ebx
    push ecx
    push edx
    push esi
    mov ecx, [ebp + 8]
    mov esi, [ebp + 12]

    ; bx = cursor position (row * 80 + col), CRTC index 0x0e = high byte, 0x0f = low byte
    mov dx, 0x3d4
    mov al, 0x0e
    out dx, al
    inc dx
    in al, dx
    mov bh, al
    dec dx
    mov al, 0x0f
    out dx, al
    inc dx
    in al, dx
    mov bl, al
    movzx ebx, bx

    test ecx, ecx
    jz .done
.print_byte:
    mov al, [esi]
    inc esi
    cmp al, 13
    je .cr
    cmp al, 10
    je .lf
    ; each cell is a character byte then an attribute byte
    mov ah, 0x07
    mov [gs:ebx * 2], ax
    inc ebx
    jmp .next
.cr:
    ; back to column 0: position = position / 80 * 80
    mov ax, bx
    mov dl, 80
    div dl
    mul dl
    mov bx, ax
    jmp .next
.lf:
    ; same column, next row
    add ebx, 80
.next:
    loop .print_byte

.done:
    ; write the position back, the cursor and the next print continue from here
    mov dx, 0x3d4
    mov al, 0x0e
    out dx, al
    inc dx
    mov al, bh
    out dx, al
    dec dx
    mov al, 0x0f
    out dx, al
    inc dx
    mov al, bl
    out dx, al

    pop esi
    pop edx
    pop ecx
    pop ebx
    pop ebp
    ret

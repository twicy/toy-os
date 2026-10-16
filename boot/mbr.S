%include "boot.inc"
SECTION MBR vstart=0x7c00
    jmp 0x0:mbr_entry

; AH=13h treats 13,10 (CR and LF) as "\n"
mbr_entry_msg:
    db "Entering MBR ...",13,10
mbr_entry_msg_len equ ($ - mbr_entry_msg)

disk_success_msg:
    db "Successfully loaded loader ...",13,10
disk_success_msg_len  equ ($ - disk_success_msg)

disk_error_msg:
    db "Disk read error",13,10
disk_error_msg_len equ ($ - disk_error_msg)

mbr_entry:
    xor ax,ax
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov sp,0x7c00
    mov fs,ax
    mov ax,0xb800
    mov gs,ax

    call clear_screen_real_mode
    mov bp,mbr_entry_msg
    mov cx,mbr_entry_msg_len
    call print_real_mode

    mov eax,LOADER_START_SECTOR
    mov bx,LOADER_BASE_ADDR
    mov cx,4
    call read_disk

    mov bp,disk_success_msg
    mov cx,disk_success_msg_len
    call print_real_mode

    jmp LOADER_BASE_ADDR

clear_screen_real_mode:
    ; scroll window up
    mov ah,0x06
    ; clear the entire window
    mov al,0x00
    ; attr for new blank lines
    mov bh,0x07
    ; top left row col
    mov ch,0x0
    mov cl,0x0
    ; bottom right row col
    mov dh,24
    mov dl,79
    int 10h
    ; set cursor position
    mov ah,0x02
    ; page 0
    mov bh,0x0
    ; row = 0, col = 0
    mov dh,0x0
    mov dl,0x0
    int 10h
    ret

; print_real_mode, halt_real_mode
%include "util.inc"

; eax = LBA of the first sector
; bx = destination buffer (ds:bx)
; cx = number of sectors to read
read_disk:
    ; save LBA and sector count, al and cl are reused below
    mov esi,eax
    mov di,cx

    ; sector count
    mov dx,0x1f2
    mov al,cl
    out dx,al

    mov eax,esi

    ; LBA bits 7..0
    mov dx,0x1f3
    out dx,al

    ; LBA bits 15..8
    mov cl,8
    shr eax,cl
    mov dx,0x1f4
    out dx,al

    ; LBA bits 23..16
    shr eax,cl
    mov dx,0x1f5
    out dx,al

    ; device register: bits 3..0 = LBA bits 27..24
    ; 0xe0: bit 6 = LBA mode, bit 4 = 0 master drive, bits 7 and 5 always 1
    shr eax,cl
    and al,0x0f
    or al,0xe0
    mov dx,0x1f6
    out dx,al

    ; command 0x20: read sectors
    mov dx,0x1f7
    mov al,0x20
    out dx,al

    ; the drive raises DRQ once per sector, wait before each one
.next_sector:
    ; reading 0x1f7 returns status
    ; status lags behind the command / last data word by up to 400ns
    ; 4 reads give the drive that time, so stale bits are not trusted
    mov dx,0x1f7
    in al,dx
    in al,dx
    in al,dx
    in al,dx
.not_ready:
    in al,dx
    ; bit 7 BSY = 1: the other bits are not valid yet
    test al,0x80
    jnz .not_ready
    ; bit 5 DF device fault, bit 0 ERR
    test al,0x21
    jnz disk_error
    ; bit 3 DRQ = 0: sector data not ready yet
    test al,0x08
    jz .not_ready

    ; data port, 256 words = one 512 byte sector
    mov dx,0x1f0
    mov cx,256
.go_on_read:
    in ax,dx
    mov [bx],ax
    add bx,2
    loop .go_on_read

    dec di
    jnz .next_sector
    ret

; no caller to return to, print and halt
disk_error:
    mov bp,disk_error_msg
    mov cx,disk_error_msg_len
    call print_real_mode
    jmp halt_real_mode

times 510-($-$$) db 0
db 0x55,0xaa

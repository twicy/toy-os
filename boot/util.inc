; %include pastes this file where the line is, so each binary gets its own copy:
; include it among functions where execution cannot fall into it,
; and in the loader before [bits 32]

%if __?BITS?__ != 16
%error "util.inc is real-mode code, include it before [bits 32]"
%endif

; cx should hold character count
; es:bp should point to the string
; prints at the cursor and leaves the cursor after the string
; 13,10 (CR LF) in the string moves to the start of the next line
print_real_mode:
    ; get cursor position, dh = row, dl = col
    ; it also returns the cursor shape in cx, so save the count
    push cx
    mov ah,0x03
    mov bh,0x0
    int 10h
    pop cx
    ; Write string
    mov ah,0x13
    mov al,0x01
    mov bh,0x0
    mov bl,0x07
    int 10h
    ret

; stop for good, for errors with no caller to return to, jmp here
halt_real_mode:
    cli
.halt:
    hlt
    jmp .halt

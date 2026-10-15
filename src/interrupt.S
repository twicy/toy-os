
[bits 32]

extern intr_dispatch

; idt stubs
; stub = error code/placeholder + handler that jumps to common intr route
global intr_stubs

%define HAS_ERRCODE(v) ((v) == 8 || ((v) >= 10 && (v) <= 14) || (v) == 17 || (v) == 21)

section .text

%assign vec 0
%rep 256
stub_%[vec]:
; If this vec has no err code, push placeholder
%if !HAS_ERRCODE(vec)
    push 0
%endif
    push vec
    jmp intr_common
%assign vec vec + 1
%endrep

intr_common:
    ; Predefined order: eax, ecx, edx, ebx, esp, ebp, esi, edi
    pushad
    push esp
    cld
    call intr_dispatch
    add esp, 4
    popad
    ; the err code/placeholder + the vec
    add esp, 8
    iretd

section .rodata
intr_stubs:
%assign vec 0
%rep 256
    dd stub_%[vec]
%assign vec vec + 1
%endrep
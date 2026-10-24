[bits 32]

global switch_to

section .text

; struct task_struct *switch_to(struct task_struct *prev, struct task_struct *next)
;
; Called by prev, returns in next. A C caller already treats eax, ecx and edx
; as destroyed by any call, and eip is the return address the call pushed, so
; only ebx, esi, edi, ebp and esp need saving. They go on prev's own stack in
; the order of struct switch_frame, and esp into prev->esp (offset 0).
;
; eax still holds prev when ret runs on next's stack, so the thread that
; resumes gets back the task that switched to it, which its own locals,
; saved long ago, cannot know.
switch_to:
    mov eax, [esp + 4]      ; prev
    mov edx, [esp + 8]      ; next
    push ebp
    push ebx
    push esi
    push edi
    mov [eax], esp          ; prev->esp = esp
    mov esp, [edx]          ; esp = next->esp, from here on this is next's stack
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret                     ; to wherever next once called switch_to, or kthread_entry

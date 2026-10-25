# Notes

## Part 1: Everything before loading the kernel

1. After CPU resets, cs:ip is forced to be 0xf000:0xfff0. The instruction there is a jump to BIOS check routine in ROM
2. BIOS enumerates, makes checks, setting up its own interrupts etc., before transferring its ownership to MBR. The transfer includes 1) searching for MBR 2) copying it over to address 0x7c00
3. MBR is a sector-sized program image that is signatured by ending with 0xaa55. Its duty is to copy the loader image over to memory. It cannot do anything more due to constraint of being at most 512 byte large.
4. The loader is no longer bounded by that constraint, therefore capable of doing many things. First, it collects the E820 memory map, while BIOS calls still work in real mode. Then it enters protected mode by setting up a provisional GDT table, and loads the kernel image from disk into a buffer (0x70000). Then it enables paging by setting up the page directory and page tables. Finally it copies the kernel out of the buffer to where it was linked and jumps to its entry.

Page tables themselves are also mapped, so that one could program it with va. The key insight here is PTE and PDE are very much alike, therefore if we put PD address into one PDE entry, it would further point to the PT0 address (as if PD holds a page table address).

Kernel images are in ELF format, essentially binaries but with extra headers. The loader reads the program headers, which describe the segments to load; the section headers are for the linker and debugger. Also ELF size != kernel size, e.g. ELF does not allocate room for BSS (p_memsz > p_filesz, the loader zeroes the difference), but kernel does. It is also loader's job to de-cipher ELF format and copy over the PT_LOAD segments (code, data) to their p_vaddr. The loader does not pick that address, the linker fixed it (-Ttext 0xc0001500 in the Makefile).

## Part 2: Kernel Infra

1. printk is implemented with gcc keywords, va_start, va_end, va_arg, etc. On i386, `va_list ap` is just a pointer into stack, `va_arg(ap,type)` reads a type at ap and moves ap to the next arg slot.

args are pushed onto stack from right to left, conforming to the system ABI calling convention. The printk process is demuxing each arg and print accordingly through vga

2. Three types of descriptors the IDT may include, we are mostly interested in Interrupt Gate and Trap Gate. Note these are what the hardware provides with us. Trap gate allows nested interrupts while Interrupt gate does not. As we know Linux exceptions can be preempted by interrupts but not vice versa, so it make sense to see that Linux exceptions are mostly registered as Trap Gate and Linux Interrupts as Interrupt Gate.

When an interrupt happens, the CPU determines the vector associated, reads idt->gdt->irq handler. There are security checks involved.

a) CPL >= idt->gdt->DPL (numerically: a lower number means more privilege). The irq handler itself cannot have a lower privilege than the program that causes the interrupt (e.g. Linux interrupt handlers are in kernel mode, ring 0, so whether an interrupts kicks in during user/kernel mode execution, it would be fine)

b) CPL <= idt->DPL. Only for programmed ones (int n, int3, into), it checks if the program is authorized to issue that. Hardware interrupts and exceptions the CPU raises itself skip this check. (e.g. User space program can issue syscalls (int 0x80) and breakpoints (int3), but not say interrupts, so syscall idt->DPL = 3, but Linux interrupt idt->DPL = 0. A real div by 0 in user space still reaches the handler, but `int $0` gets a #GP, since vector 0's idt->DPL = 0)

The CPU optionally switch to a new stack if there is an escalation from CPL to idt->gdt->DPL.

It then saves (+user_ss, user_esp if any) + eflags, cs and eip (+error code if any) in the stack, set out for the irq handler

On the software side, vector, eax ... edi are all pushed onto stack and fed to the handler.
iretd is called to further restore cs:eip and user_ss:user_esp if necessary.

3. buddy system, slab, kmalloc. I think the code speaks for itself. vmalloc uses guard pages for cheap safety checks. The guard pages only consume VA space and not PA space (PTEs not installed)

4. double fault. Why would double fault emerge? threads has a kernel stack, that is vmalloced by 2-page-size, with an unmapped guard page below it. An overflow touches the guard page, which is a page fault. The CPU then pushes the page fault's frame (eflags, cs, eip, error code) onto that same broken stack, which faults again while still delivering the first fault: that is the double fault. If delivering the double fault faulted a third time (e.g. an interrupt gate pushing onto the same stack once more), it would be a triple fault: the CPU shuts down and the machine resets. It stops there, there is no fourth fault.

How are double fault handled? Vector 8 is a task gate: it holds a selector to a TSS descriptor in the GDT, and performs a hardware task switch rather than just pushing a frame. The CPU saves the faulting state into kernel_tss (the TSS in TR) and loads everything, esp included, from doublefault_tss, so the handler runs on a fresh stack of its own.

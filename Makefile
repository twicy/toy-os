IMG := toy-os.img

.PHONY: all run clean

# Write all stages into the disk image. The seeks must match
# LOADER_START_SECTOR and KERNEL_START_SECTOR in boot.inc, the loader
# count must match the sector count (mov cx) the MBR reads, and the
# kernel count must match KERNEL_SECTOR_COUNT.
all: mbr.bin loader.bin kernel.bin
	dd if=./mbr.bin of=./$(IMG) bs=512 count=1 conv=notrunc
	dd if=./loader.bin of=./$(IMG) bs=512 count=4 seek=2 conv=notrunc
	dd if=./kernel.bin of=./$(IMG) bs=512 count=200 seek=9 conv=notrunc

%.bin: %.S boot.inc
	nasm -f bin -o $@ $<

# The host compiler targets x86-64, -m32 / -m elf_i386 build for the
# 32-bit protected mode the loader runs in.
# -fno-pie: plain absolute addresses, no GOT.
# -fno-stack-protector: the canary is read from gs:0x14, but gs is the
# video segment here, and __stack_chk_fail does not exist.
main.o: main.c
	gcc -m32 -fno-pie -fno-stack-protector -c -o $@ $<

# -Ttext must match KERNEL_ENTRY_POINT in boot.inc
kernel.bin: main.o
	ld -m elf_i386 $< -Ttext 0xc0001500 -e main -o $@

run: all
	bochs -f bochs_setup.txt

clean:
	rm -f mbr.bin loader.bin main.o kernel.bin

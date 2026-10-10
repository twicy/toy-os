IMG := toy-os.img

.PHONY: all run clean

# Write both stages into the disk image. The seek must match
# LOADER_START_SECTOR in boot.inc, and the loader count must match
# the sector count (mov cx) the MBR reads.
all: mbr.bin loader.bin
	dd if=./mbr.bin of=./$(IMG) bs=512 count=1 conv=notrunc
	dd if=./loader.bin of=./$(IMG) bs=512 count=4 seek=2 conv=notrunc

%.bin: %.S boot.inc
	nasm -f bin -o $@ $<

run: all
	bochs -f bochs_setup.txt

clean:
	rm -f mbr.bin loader.bin

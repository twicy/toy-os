IMG := toy-os.img
BUILD := build

.PHONY: all check-image run run-x11 clean

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

# Bochs refuses an image another Bochs has open (image locked). Fail
# early with its pid, before all rewrites the image under it. A lock left
# while nothing has the image open is stale, from a Bochs that did not
# exit cleanly (e.g. its terminal was closed), and is removed.
check-image:
	@if pids=$$(fuser $(IMG) 2>/dev/null); then echo "$(IMG) is open in another Bochs (pid$$pids), quit it first: Ctrl-C, then q"; exit 1; fi
	@rm -f $(IMG).lock

# bochs_setup.txt serves the screen over VNC, Remmina shows it scaled to
# its window. Bochs takes the first free port from 5900 up but Remmina
# expects 5900, so it must be free. Remmina is started once Bochs listens,
# Bochs waits up to 30s for it, and the debugger keeps this terminal.
# Ctrl-C (break into the debugger) reaches this shell too, the no-op trap
# keeps it waiting for Bochs instead of exiting. Remmina is quit together
# with Bochs, unless it was already open before (remmina --quit exits 1
# even when it worked).
run: check-image all $(BUILD)/toy-os.remmina
	@if ss -tln | grep -q ':5900 '; then echo "port 5900 is in use, Bochs would not get it"; exit 1; fi
	trap : INT; \
	remmina_was_open=$$(pgrep -x remmina); \
	(timeout 30 sh -c 'until ss -tln | grep -q ":5900 "; do sleep 0.2; done' && \
	 remmina -c $(CURDIR)/$(BUILD)/toy-os.remmina >/dev/null 2>&1) & \
	bochs -f bochs_setup.txt; \
	if [ -z "$$remmina_was_open" ]; then remmina --quit >/dev/null 2>&1 || true; fi

# Remmina saves its window size into the profile it opens, so it gets a
# copy, refreshed only when toy-os.remmina itself changes
$(BUILD)/toy-os.remmina: toy-os.remmina
	@mkdir -p $(dir $@)
	cp $< $@

# The old way: Bochs in its own X11 window (unscaled) with the X11 keymap.
# Options after -f override bochs_setup.txt.
run-x11: check-image all
	bochs -f bochs_setup.txt 'display_library: x' 'keyboard: keymap=/usr/share/bochs/keymaps/x11-pc-us.map'

clean:
	rm -rf mbr.bin loader.bin main.o kernel.bin $(BUILD)

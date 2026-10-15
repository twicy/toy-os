IMG := toy-os.img
BUILD := build

KERNEL_SRCS := main.c $(wildcard src/*.c lib/*.c)
KERNEL_ASMS := $(wildcard src/*.S)
KERNEL_OBJS := $(KERNEL_SRCS:%.c=$(BUILD)/%.o) $(KERNEL_ASMS:%.S=$(BUILD)/%.o)

.PHONY: all check-image run run-x11 clean

all: mbr.bin loader.bin kernel.bin
	dd if=./mbr.bin of=./$(IMG) bs=512 count=1 conv=notrunc
	dd if=./loader.bin of=./$(IMG) bs=512 count=4 seek=2 conv=notrunc
	dd if=./kernel.bin of=./$(IMG) bs=512 count=200 seek=9 conv=notrunc

%.bin: boot/%.S boot/boot.inc
	nasm -f bin -i boot/ -o $@ $<

CFLAGS := -m32 -ffreestanding -nostdinc -Iinclude \
          -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables \
          -Wall -Wextra -MMD -MP

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	gcc $(CFLAGS) -c -o $@ $<

$(BUILD)/%.o: %.S
	@mkdir -p $(dir $@)
	nasm -f elf32 -o $@ $<

kernel.bin: $(KERNEL_OBJS)
	ld -m elf_i386 $^ -Ttext 0xc0001500 -e main -o $@

-include $(KERNEL_OBJS:.o=.d)

check-image:
	@if pids=$$(fuser $(IMG) 2>/dev/null); then echo "$(IMG) is open in another Bochs (pid$$pids), quit it first: Ctrl-C, then q"; exit 1; fi
	@rm -f $(IMG).lock

run: check-image all $(BUILD)/toy-os.remmina
	@if ss -tln | grep -q ':5900 '; then echo "port 5900 is in use, Bochs would not get it"; exit 1; fi
	trap : INT; \
	remmina_was_open=$$(pgrep -x remmina); \
	(timeout 30 sh -c 'until ss -tln | grep -q ":5900 "; do sleep 0.2; done' && \
	 remmina -c $(CURDIR)/$(BUILD)/toy-os.remmina >/dev/null 2>&1) & \
	bochs -f bochs_setup.txt; \
	if [ -z "$$remmina_was_open" ]; then remmina --quit >/dev/null 2>&1 || true; fi

$(BUILD)/toy-os.remmina: toy-os.remmina
	@mkdir -p $(dir $@)
	cp $< $@

# The old way: Bochs in its own X11 window (unscaled) with the X11 keymap.
run-x11: check-image all
	bochs -f bochs_setup.txt 'display_library: x' 'keyboard: keymap=/usr/share/bochs/keymaps/x11-pc-us.map'

clean:
	rm -rf mbr.bin loader.bin kernel.bin $(BUILD)

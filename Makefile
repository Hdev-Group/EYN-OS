COMPILER = gcc
LINKER = ld
ASSEMBLER = nasm
CFLAGS = -m32 -c -ffreestanding -w -fcommon -Oz
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T src/boot/link.ld
EMULATOR = qemu-system-i386
EMULATOR_FLAGS = -kernel

OBJS = obj/kasm.o obj/kc.o obj/idt.o obj/isr.o obj/kb.o obj/string.o obj/system.o obj/util.o obj/shell.o obj/math.o obj/vga.o obj/fat32.o obj/ata.o
OUTPUT = tmp/boot/kernel.bin

all:$(OBJS)
	mkdir tmp/ -p
	mkdir tmp/boot/ -p
	$(LINKER) $(LDFLAGS) -o $(OUTPUT) $(OBJS)

obj/kasm.o:src/boot/kernel.asm
	mkdir obj/ -p
	$(ASSEMBLER) $(ASFLAGS) -o obj/kasm.o src/boot/kernel.asm
	
obj/kc.o:src/entry/kernel.c
	$(COMPILER) $(CFLAGS) src/entry/kernel.c -o obj/kc.o 
	
obj/idt.o:src/cpu/idt.c
	$(COMPILER) $(CFLAGS) src/cpu/idt.c -o obj/idt.o 

obj/kb.o:src/drivers/kb.c
	$(COMPILER) $(CFLAGS) src/drivers/kb.c -o obj/kb.o

obj/isr.o:src/cpu/isr.c
	$(COMPILER) $(CFLAGS) src/cpu/isr.c -o obj/isr.o

obj/string.o:src/utilities/shell/string.c
	$(COMPILER) $(CFLAGS) src/utilities/shell/string.c -o obj/string.o

obj/system.o:src/cpu/system.c
	$(COMPILER) $(CFLAGS) src/cpu/system.c -o obj/system.o

obj/util.o:src/utilities/util.c
	$(COMPILER) $(CFLAGS) src/utilities/util.c -o obj/util.o
	
obj/shell.o:src/utilities/shell/shell.c
	$(COMPILER) $(CFLAGS) src/utilities/shell/shell.c -o obj/shell.o

obj/math.o:src/utilities/basic/math.c
	$(COMPILER) $(CFLAGS) src/utilities/basic/math.c -o obj/math.o

obj/vga.o:src/drivers/vga.c
	$(COMPILER) $(CFLAGS) src/drivers/vga.c -o obj/vga.o

obj/fat32.o:src/drivers/fat32.c
	$(COMPILER) $(CFLAGS) src/drivers/fat32.c -o obj/fat32.o

obj/ata.o:src/drivers/ata.c
	$(COMPILER) $(CFLAGS) src/drivers/ata.c -o obj/ata.o

fat32img:
	rm -f disk.img
	dd if=/dev/zero of=disk.img bs=1M count=4
	mkfs.fat -F 32 disk.img
	# Copy all source and header files to the root of the disk image
	mcopy -i disk.img src/drivers/*.c ::/
	mcopy -i disk.img src/boot/*.asm ::/
	mcopy -i disk.img src/boot/*.ld ::/
	mcopy -i disk.img src/utilities/*.c ::/
	mcopy -i disk.img src/utilities/shell/*.c ::/
	mcopy -i disk.img src/utilities/basic/*.c ::/
	mcopy -i disk.img src/cpu/*.c ::/
	mcopy -i disk.img src/entry/*.c ::/
	mcopy -i disk.img include/*.h ::/
	mcopy -i disk.img README.md ::/
	mcopy -i disk.img LICENSE ::/
	mkdir -p tmp/boot/
	mv disk.img tmp/boot/

build: all fat32img
	grub-mkrescue -o EYNOS.iso tmp/
	
clear:
	rm -f obj/*.o
	rm -r -f tmp/boot/kernel.bin
	
COMPILER = gcc
LINKER = ld
ASSEMBLER = nasm
CFLAGS = -m32 -c -ffreestanding -w -fcommon -Oz -fno-stack-protector
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T src/boot/link.ld
EMULATOR = qemu-system-i386
EMULATOR_FLAGS = -kernel

OBJS = obj/kasm.o obj/kc.o obj/idt.o obj/isr.o obj/kb.o obj/string.o obj/system.o obj/util.o obj/shell.o obj/math.o obj/vga.o obj/fat32.o obj/ata.o obj/eynfs.o obj/shell_commands.o obj/fs_commands.o obj/fdisk_commands.o obj/format_command.o obj/write_editor.o obj/tui.o obj/help_tui.o obj/assemble.o obj/run_command.o obj/history.o obj/game_engine.o
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

obj/eynfs.o:src/drivers/eynfs.c
	$(COMPILER) $(CFLAGS) src/drivers/eynfs.c -o obj/eynfs.o

obj/shell_commands.o:src/utilities/shell/shell_commands.c
	$(COMPILER) $(CFLAGS) src/utilities/shell/shell_commands.c -o obj/shell_commands.o

obj/fs_commands.o:src/utilities/shell/fs_commands.c
	$(COMPILER) $(CFLAGS) src/utilities/shell/fs_commands.c -o obj/fs_commands.o

obj/fdisk_commands.o:src/utilities/shell/fdisk_commands.c
	$(COMPILER) $(CFLAGS) src/utilities/shell/fdisk_commands.c -o obj/fdisk_commands.o

obj/format_command.o:src/utilities/shell/format_command.c
	$(COMPILER) $(CFLAGS) src/utilities/shell/format_command.c -o obj/format_command.o

obj/write_editor.o:src/utilities/shell/write_editor.c
	$(COMPILER) $(CFLAGS) src/utilities/shell/write_editor.c -o obj/write_editor.o

obj/run_command.o:src/utilities/shell/run_command.c
	$(COMPILER) $(CFLAGS) src/utilities/shell/run_command.c -o obj/run_command.o

obj/history.o:src/utilities/shell/history.c
	$(COMPILER) $(CFLAGS) src/utilities/shell/history.c -o obj/history.o

obj/compile_command.o:src/utilities/shell/compile_command.c
	$(COMPILER) $(CFLAGS) src/utilities/shell/compile_command.c -o obj/compile_command.o

obj/tui.o:src/utilities/tui/tui.c
	$(COMPILER) $(CFLAGS) src/utilities/tui/tui.c -o obj/tui.o

obj/help_tui.o:src/utilities/shell/help_tui.c
	$(COMPILER) $(CFLAGS) src/utilities/shell/help_tui.c -o obj/help_tui.o

obj/assemble.o:src/utilities/shell/assemble.c
	$(COMPILER) $(CFLAGS) src/utilities/shell/assemble.c -o obj/assemble.o

obj/game_engine.o:src/utilities/games/game_engine.c
	$(COMPILER) $(CFLAGS) src/utilities/games/game_engine.c -o obj/game_engine.o

fat32img:
	rm -f disk.img
	dd if=/dev/zero of=disk.img bs=1M count=5
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

build: all eynfsimg fat32img
	mkdir -p tmp/grub_ultra_minimal/boot/grub
	cp tmp/boot/kernel.bin tmp/grub_ultra_minimal/boot/
	cp disk.img tmp/grub_ultra_minimal/boot/
	@echo 'set default=0' > tmp/grub_ultra_minimal/boot/grub/grub.cfg
	@echo 'set timeout=0' >> tmp/grub_ultra_minimal/boot/grub/grub.cfg
	@echo 'set gfxmode=text' >> tmp/grub_ultra_minimal/boot/grub/grub.cfg
	@echo '' >> tmp/grub_ultra_minimal/boot/grub/grub.cfg
	@echo 'menuentry "EYN-OS" {' >> tmp/grub_ultra_minimal/boot/grub/grub.cfg
	@echo '    multiboot /boot/kernel.bin' >> tmp/grub_ultra_minimal/boot/grub/grub.cfg
	@echo '    module /boot/disk.img' >> tmp/grub_ultra_minimal/boot/grub/grub.cfg
	@echo '    boot' >> tmp/grub_ultra_minimal/boot/grub/grub.cfg
	@echo '}' >> tmp/grub_ultra_minimal/boot/grub/grub.cfg
	grub-mkrescue --modules="multiboot multiboot2 part_msdos ext2 iso9660" --locales="" --themes="" --fonts="" --compress=xz -o EYNOS.iso tmp/grub_ultra_minimal/
	@echo "Ultra-minimal ISO created: EYNOS.iso"
	@ls -lh EYNOS.iso
	@echo "Removing EFI files from ISO..."
	mkdir -p /tmp/iso_edit
	sudo mount -o loop EYNOS.iso /tmp/iso_edit
	mkdir -p /tmp/iso_clean
	cp -r /tmp/iso_edit/* /tmp/iso_clean/
	rm -rf /tmp/iso_clean/efi*
	rm -rf /tmp/iso_clean/boot/grub/i386-efi
	rm -rf /tmp/iso_clean/boot/grub/x86_64-efi
	sudo umount /tmp/iso_edit
	xorriso -as mkisofs -o EYNOS.iso -b boot/grub/i386-pc/eltorito.img -no-emul-boot -boot-load-size 4 -boot-info-table --grub2-boot-info --grub2-mbr /usr/lib/grub/i386-pc/boot_hybrid.img -r -V "EYN-OS" -iso-level 3 -joliet-long /tmp/iso_clean
	rm -rf /tmp/iso_clean
	@echo "Ultra-minimal ISO created: EYNOS.iso"
	@ls -lh EYNOS.iso

clean:
	rm -rf obj/*.o tmp/boot/kernel.bin eynfs.img disk.img eynfs_format EYNOS.iso
	rm -rf tmp/grub_minimal tmp/grub_ultra_minimal
	rm -f userland/*.o userland/*.bin

clear: clean

# Build the userland EYNFS format tool
eynfs_format: eynfs_format.c include/eynfs.h
	$(COMPILER) -o eynfs_format eynfs_format.c

# Create and format a 5MB EYNFS disk image
eynfsimg: eynfs_format
	rm -f eynfs.img
	dd if=/dev/zero of=eynfs.img bs=1M count=5
	$(COMPILER) -o eynfs_format eynfs_format.c
	./eynfs_format eynfs.img
	python3 devtools/copy_testdir_to_eynfs.py

run: build
	qemu-system-i386 -cdrom EYNOS.iso \
	  -hda eynfs.img \
	  -boot d

test:
	qemu-system-i386 -cdrom EYNOS.iso \
	  -hda eynfs.img \
	  -boot d
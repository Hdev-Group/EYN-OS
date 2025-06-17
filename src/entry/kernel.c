#include "../../include/kb.h"
#include "../../include/isr.h"
#include "../../include/idt.h"
#include "../../include/util.h"
#include "../../include/shell.h"
#include "../../include/tty.h"
#include "../../include/vga.h"
#include "../../include/multiboot.h"

int kmain(uint32 magic, multiboot_info_t *mbi)
{
	(void) magic;

	isr_install();
	clearScreen();
	printf_colored("EYN-OS v0.04\nType 'help' for a list of commands.\n\n", 7, 0);
  	launch_shell(0, mbi);
}

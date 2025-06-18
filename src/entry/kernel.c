#include "../../include/kb.h"
#include "../../include/isr.h"
#include "../../include/idt.h"
#include "../../include/util.h"
#include "../../include/shell.h"
#include "../../include/vga.h"
#include "../../include/multiboot.h"

int kmain(uint32 magic, multiboot_info_t *mbi)
{
	(void) magic;

	isr_install(mbi);
	clearScreen(mbi);
	printf(mbi, "EYN-OS v0.05\nType 'help' for a list of commands.\n\n", 200, 200, 200);
  	launch_shell(0, mbi);
	while(1)
	{
	}
}

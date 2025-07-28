#include "../../include/kb.h"
#include "../../include/isr.h"
#include "../../include/idt.h"
#include "../../include/util.h"
#include "../../include/shell.h"
#include "../../include/vga.h"
#include "../../include/multiboot.h"
#include "../../include/fat32.h"
#include "../../include/system.h"

void* fat32_disk_img = 0;
multiboot_info_t *g_mbi = 0;

// Forward declaration for ATA driver functions
extern void ata_init_drives(void);

int kmain(uint32 magic, multiboot_info_t *mbi)
{
	g_mbi = mbi;
	if (mbi->flags & MULTIBOOT_INFO_MODS && mbi->mods_count > 0) {
		multiboot_module_t* mods = (multiboot_module_t*)mbi->mods_addr;
		fat32_disk_img = (void*)mods[0].mod_start;
	}

	isr_install();
	clearScreen();
	


	// Initialize ATA drives
	ata_init_drives();
	
	printf("EYN-OS Release 11\nType 'help' for a list of commands.\n\n");

	// Launch shell
	launch_shell(0);
	
	return 0;
}

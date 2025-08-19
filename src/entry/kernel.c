#include <kb.h>
#include <isr.h>
#include <idt.h>
#include <util.h>
#include <shell.h>
#include <vga.h>
#include <multiboot.h>
#include <fat32.h>
#include <system.h>
#include <predictive_memory.h>
#include <zero_copy.h>

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

	// Full initialization - all services
	isr_install();
	clearScreen();
	
	printf("EYN-OS Release 13\n");
	printf("Type 'help' for a list of commands.\n\n");
	
	// Initialize ATA drives immediately
	ata_init_drives();

	// Initialize predictive memory management system
	predictive_memory_init();
	
	// Initialize zero-copy file operations system
	zero_copy_init();
	
	// Launch shell with full initialization
	launch_shell(0);
	
	return 0;
}

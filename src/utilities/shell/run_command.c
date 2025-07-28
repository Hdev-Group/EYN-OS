#include "../../../include/types.h"
#include "../../../include/eyn_exe_format.h"
#include "../../../include/vga.h"
#include "../../../include/fs_commands.h"
#include "../../../include/shell_command_info.h"
#include "../../../include/util.h"
#include "../../../include/string.h"
#include "../../../include/eynfs.h"

#define EYNFS_SUPERBLOCK_LBA 2048
#define USER_CODE_ADDR 0x200000

typedef void (*user_entry_t)(void);

// List of dangerous opcodes to check for
static const uint8_t dangerous_opcodes[] = {
    0xF4, // hlt
    0xFA, // cli
    0xFB, // sti
    0xCD, // int
    0xE4, 0xEC, 0xE5, 0xED, // in
    0xE6, 0xEE, 0xE7, 0xEF  // out
};

static int contains_dangerous_opcode(const uint8_t* code, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        for (size_t j = 0; j < sizeof(dangerous_opcodes); ++j) {
            if (code[i] == dangerous_opcodes[j]) {
                printf("[run] Dangerous opcode 0x%X found at offset %d\n", code[i], (int)i);
                return 1;
            }
        }
    }
    return 0;
}

void run_command(string arg) {
    // Parse filename
    uint8 i = 0;
    while (arg[i] && arg[i] != ' ') i++;
    while (arg[i] && arg[i] == ' ') i++;
    if (!arg[i]) {
        printf("Usage: run <program.eyn>\n");
        return;
    }
    char filename[64];
    uint8 j = 0;
    while (arg[i] && arg[i] != ' ' && j < 63) filename[j++] = arg[i++];
    filename[j] = 0;
    printf("[run] Loading '%s'...\n", filename);

    // Read file
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(0, EYNFS_SUPERBLOCK_LBA, &sb) != 0 || sb.magic != EYNFS_MAGIC) {
        printf("[run] No supported filesystem found.\n");
        return;
    }
    eynfs_dir_entry_t entry;
    if (eynfs_find_in_dir(0, &sb, sb.root_dir_block, filename, &entry, 0) != 0) {
        printf("[run] File not found: %s\n", filename);
        return;
    }
    uint32_t size = entry.size;
    char* buf = (char*)my_malloc(size);
    if (!buf) {
        printf("[run] Out of memory.\n");
        return;
    }
    int n = eynfs_read_file(0, &sb, &entry, buf, size, 0);
    if (n < 0) {
        printf("[run] Failed to read file: %s\n", filename);
        my_free(buf);
        return;
    }
    // Parse header
    if (size < sizeof(struct eyn_exe_header)) {
        printf("[run] File too small to be a valid EYN executable.\n");
        my_free(buf);
        return;
    }
    struct eyn_exe_header* hdr = (struct eyn_exe_header*)buf;
    if (hdr->magic[0] != 'E' || hdr->magic[1] != 'Y' || hdr->magic[2] != 'N' || hdr->magic[3] != '\0') {
        printf("[run] Invalid EYN executable magic.\n");
        my_free(buf);
        return;
    }
    printf("[run] EYN executable detected. Code size: %u, Entry: 0x%X\n", hdr->code_size, hdr->entry_point);
    if (size < sizeof(struct eyn_exe_header) + hdr->code_size) {
        printf("[run] File truncated (code section incomplete).\n");
        my_free(buf);
        return;
    }
    // Static check for dangerous opcodes
    const uint8_t* code = (const uint8_t*)(buf + sizeof(struct eyn_exe_header));
    if (contains_dangerous_opcode(code, hdr->code_size)) {
        printf("[run] Refusing to run: dangerous instructions detected.\n");
        my_free(buf);
        return;
    }
    // Copy code to fixed address
    printf("[run] Copying code to 0x%X...\n", USER_CODE_ADDR);
    memory_copy((char*)code, (char*)USER_CODE_ADDR, hdr->code_size);
    // Optionally zero out data, set up stack, etc.
    // Jump to entry point
    printf("[run] Jumping to entry point at 0x%X...\n", USER_CODE_ADDR + hdr->entry_point);
    user_entry_t entry_fn = (user_entry_t)(USER_CODE_ADDR + hdr->entry_point);
    entry_fn();
    printf("[run] Program returned.\n");
    my_free(buf);
}

REGISTER_SHELL_COMMAND(run, "run", "Run a .eyn executable with static safety checks.\nUsage: run <program.eyn>", "run test.eyn"); 
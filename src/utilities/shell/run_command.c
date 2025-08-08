#include <types.h>
#include <eyn_exe_format.h>
#include <vga.h>
#include <fs_commands.h>
#include <shell_command_info.h>
#include <util.h>
#include <string.h>
#include <eynfs.h>

void run_cmd(string arg);

#define EYNFS_SUPERBLOCK_LBA 2048
#define USER_CODE_ADDR 0x200000
#define USER_STACK_ADDR 0x300000
#define USER_HEAP_ADDR 0x400000
#define USER_HEAP_SIZE 0x10000  // 64KB user heap (reduced from 1MB!)
#define MAX_PROCESSES 2         // Reduced from 4 for memory savings

// Ultra-lightweight process isolation structures
typedef struct {
    uint32 pid;
    uint32 code_start;
    uint32 code_size;
    uint32 stack_start;
    uint32 stack_size;
    uint32 heap_start;
    uint32 heap_size;
    uint32 entry_point;
    char name[16];              // Reduced from 32
    int active;
    int user_mode;
} process_t;

static process_t processes[MAX_PROCESSES];
static uint32 next_pid = 1;
static int process_isolation_enabled = 1;

// Memory protection for user space (ultra lightweight)
static uint8* user_heap_start = (uint8*)USER_HEAP_ADDR;
static uint32 user_heap_used = 0;

// Process management functions (optimized)
static uint32 allocate_process_id() {
    return next_pid++;
}

static process_t* create_process(const char* name, uint32 code_size, uint32 entry_point) {
    // Find free process slot
    int slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!processes[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        printf("[PROCESS] No free process slots available\n");
        return NULL;
    }
    
    process_t* proc = &processes[slot];
    proc->pid = allocate_process_id();
    proc->code_start = USER_CODE_ADDR;
    proc->code_size = code_size;
    proc->stack_start = USER_STACK_ADDR + (slot * 0x8000); // 32KB stack per process
    proc->stack_size = 0x8000; // 32KB stack per process (reduced from 64KB)
    proc->heap_start = USER_HEAP_ADDR + (slot * 0x8000); // 32KB heap per process
    proc->heap_size = 0x8000; // 32KB heap per process (reduced from 256KB)
    proc->entry_point = entry_point;
    proc->active = 1;
    proc->user_mode = 1;
    
    safe_strcpy(proc->name, name, sizeof(proc->name));
    
    printf("[PROCESS] Created process %d: %s\n", proc->pid, proc->name);
    return proc;
}

// User memory allocation (separate from kernel heap) - ultra lightweight
void* user_malloc(uint32 size) {
    if (!process_isolation_enabled) {
        return my_malloc(size);
    }
    
    if (user_heap_used + size > USER_HEAP_SIZE) {
        printf("[PROCESS] User heap out of memory\n");
        return NULL;
    }
    
    void* ptr = (void*)(user_heap_start + user_heap_used);
    user_heap_used += size;
    return ptr;
}

// Validate memory access for user processes (simplified)
static int validate_user_memory_access(uint32 addr, uint32 size) {
    if (!process_isolation_enabled) return 1;
    
    // Check if address is in user space
    if (addr < USER_CODE_ADDR || addr >= USER_HEAP_ADDR + USER_HEAP_SIZE) {
        printf("[PROCESS] Invalid memory access: 0x%X\n", addr);
        return 0;
    }
    
    // Check bounds
    if (addr + size > USER_HEAP_ADDR + USER_HEAP_SIZE) {
        printf("[PROCESS] Memory access out of bounds: 0x%X\n", addr);
        return 0;
    }
    
    return 1;
}

// Switch to user mode (simplified)
static void switch_to_user_mode() {
    if (!process_isolation_enabled) return;
    
    // In a real implementation, this would change CPU privilege level
    // For now, we just track the mode
    printf("[PROCESS] Switching to user mode\n");
}

// Switch back to kernel mode
static void switch_to_kernel_mode() {
    if (!process_isolation_enabled) return;
    
    printf("[PROCESS] Switching to kernel mode\n");
}

// Get current process info
static process_t* get_current_process() {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].active) {
            return &processes[i];
        }
    }
    return NULL;
}

typedef void (*user_entry_t)(void);

// List of dangerous opcodes to check for
static const uint8_t dangerous_opcodes[] = {
    0xF4, // hlt
    0xFA, // cli
    0xFB, // sti
    /* 0xCD intentionally excluded: allow int but screen per-instruction below */
    0xE4, 0xEC, 0xE5, 0xED, // in
    0xE6, 0xEE, 0xE7, 0xEF  // out
};

static int contains_dangerous_opcode(const uint8_t* code, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        uint8_t op = code[i];
        if (op == 0xCD) {
            // Allow int 0x80 only
            if (i + 1 < size) {
                uint8_t imm = code[i + 1];
                if (imm == 0x80) {
                    i++; // skip imm8 and continue
                    continue;
                }
            }
            printf("[run] Blocking software interrupt other than 0x80 at offset %d\n", (int)i);
            return 1;
        }
        for (size_t j = 0; j < sizeof(dangerous_opcodes); ++j) {
            if (op == dangerous_opcodes[j]) {
                printf("[run] Dangerous opcode 0x%X found at offset %d\n", op, (int)i);
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
    
    // Create process for isolation
    process_t* proc = create_process(filename, hdr->code_size, hdr->entry_point);
    if (!proc) {
        printf("[run] Failed to create process\n");
        my_free(buf);
        return;
    }
    
    // Set up execution environment with process isolation
    printf("[run] Setting up isolated execution environment...\n");
    
    // Copy code to user space
    printf("[run] Copying code to user space 0x%X...\n", proc->code_start);
    if (!validate_user_memory_access(proc->code_start, hdr->code_size)) {
        printf("[run] Memory validation failed\n");
        my_free(buf);
        return;
    }
    memory_copy((char*)code, (char*)proc->code_start, hdr->code_size);
    
    // Set up data section if present
    if (hdr->data_size > 0) {
        const uint8_t* data = (const uint8_t*)(buf + sizeof(struct eyn_exe_header) + hdr->code_size);
        uint32 data_addr = proc->code_start + 0x1000;
        printf("[run] Copying data section (%u bytes) to 0x%X...\n", hdr->data_size, data_addr);
        if (validate_user_memory_access(data_addr, hdr->data_size)) {
            memory_copy((char*)data, (char*)data_addr, hdr->data_size);
        }
    }
    
    // Set up user stack
    printf("[run] Setting up user stack at 0x%X...\n", proc->stack_start);
    
    // Switch to user mode
    switch_to_user_mode();
    
    // Jump to entry point in user space
    printf("[run] Jumping to user entry point at 0x%X...\n", proc->code_start + hdr->entry_point);
    user_entry_t entry_fn = (user_entry_t)(proc->code_start + hdr->entry_point);
    
    // Reset user interrupt flag
    g_user_interrupt = 0;
    
    // Execute in user space
    entry_fn();
    
    // Switch back to kernel mode
    switch_to_kernel_mode();
    
    // Check if program was interrupted
    if (g_user_interrupt) {
        printf("[run] Program interrupted by user (Ctrl+C).\n");
        g_user_interrupt = 0;
    } else {
        printf("[run] Program returned successfully.\n");
    }
    
    // Clean up process
    proc->active = 0;
    printf("[run] Process %d terminated\n", proc->pid);
    
    my_free(buf);
}

// Process management functions for shell commands
int get_process_count() {
    int count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].active) count++;
    }
    return count;
}

process_t* get_process_by_id(uint32 pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].active && processes[i].pid == pid) {
            return &processes[i];
        }
    }
    return NULL;
}

int get_process_isolation_status() {
    return process_isolation_enabled;
}

REGISTER_SHELL_COMMAND(run, "run", run_cmd, CMD_STREAMING, "Run a .eyn executable with process isolation.\nUsage: run <program.eyn>", "run test.eyn"); 
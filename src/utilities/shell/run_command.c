#include <run_command.h>
#include <eynfs.h>
#include <util.h>
#include <types.h>
#include <isr.h>
#include <eyn_exe_format.h>
#include <vga.h>
#include <fs_commands.h>
#include <shell_command_info.h>
#include <string.h>

// Function declarations
extern uint32_t syscall_dispatch(regs_t* r);

void run_cmd(string arg);

// Process management
#define MAX_PROCESSES 16
#define USER_CODE_ADDR 0x2000000  // 32MB
#define USER_STACK_ADDR 0x3000000 // 48MB
#define USER_HEAP_ADDR 0x4000000  // 64MB
#define USER_HEAP_SIZE 0x10000    // 64KB user heap
#define EYNFS_SUPERBLOCK_LBA 2048

// Global process table
static process_t g_processes[MAX_PROCESSES];
static int g_next_pid = 1;
static int process_isolation_enabled = 1;

// Memory protection for user space (ultra lightweight)
static uint8* user_heap_start = (uint8*)USER_HEAP_ADDR;
static uint32 user_heap_used = 0;

// Process management functions (optimized)
static uint32 allocate_process_id() {
    return g_next_pid++;
}

static process_t* create_process(const char* name, uint32 code_size, uint32 entry_point) {
    // Find free process slot
    int slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!g_processes[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        printf("[PROCESS] No free process slots available\n");
        return NULL;
    }
    
    process_t* proc = &g_processes[slot];
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
        return malloc(size);
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
        if (g_processes[i].active) {
            return &g_processes[i];
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
    char* buf = (char*)malloc(size);
    if (!buf) {
        printf("[run] Out of memory.\n");
        return;
    }
    int n = eynfs_read_file(0, &sb, &entry, buf, size, 0);
    if (n < 0) {
        printf("[run] Failed to read file: %s\n", filename);
        free(buf);
        return;
    }
    // Parse header
    if (size < sizeof(struct eyn_exe_header)) {
        printf("[run] File too small to be a valid EYN executable.\n");
        free(buf);
        return;
    }
    struct eyn_exe_header* hdr = (struct eyn_exe_header*)buf;
    if (hdr->magic[0] != 'E' || hdr->magic[1] != 'Y' || hdr->magic[2] != 'N' || hdr->magic[3] != '\0') {
        printf("[run] Invalid EYN executable magic.\n");
        free(buf);
        return;
    }
    printf("[run] EYN executable detected. Code size: %d, Data size: %d, Entry offset: %d\n", 
           (int)hdr->code_size, (int)hdr->data_size, (int)hdr->entry_point);
    if (size < sizeof(struct eyn_exe_header) + hdr->code_size) {
        printf("[run] File truncated (code section incomplete).\n");
        free(buf);
        return;
    }
    // Static check for dangerous opcodes
    uint8_t* code = (uint8_t*)(buf + sizeof(struct eyn_exe_header));
    if (contains_dangerous_opcode(code, hdr->code_size)) {
        printf("[run] Refusing to run: dangerous instructions detected.\n");
        free(buf);
        return;
    }
    
    // Lightweight relocation pass: fix absolute references to .data (0x1000 + offset)
    // Some assembler builds encode data labels as 0x00001000 + offset rather than absolute
    // runtime addresses. Convert those immediates in MOV r32, imm32 to code_base + 0x1000 + offset.
    // Pattern: 0xB8..0xBF imm32
    uint32 code_base = USER_CODE_ADDR;
    int reloc_count = 0;
    for (uint32 i = 0; i + 5 <= hdr->code_size; ) {
        uint8 op = code[i];
        if (op >= 0xB8 && op <= 0xBF && i + 5 <= hdr->code_size) {
            uint32 imm = (uint32)code[i+1] | ((uint32)code[i+2] << 8) | ((uint32)code[i+3] << 16) | ((uint32)code[i+4] << 24);
            // If immediate falls within the 0x1000..0x1000+data_size range, relocate
            if (imm >= 0x1000 && imm < 0x1000 + hdr->data_size) {
                uint32 relocated = code_base + imm;
                code[i+1] = (uint8)(relocated & 0xFF);
                code[i+2] = (uint8)((relocated >> 8) & 0xFF);
                code[i+3] = (uint8)((relocated >> 16) & 0xFF);
                code[i+4] = (uint8)((relocated >> 24) & 0xFF);
                reloc_count++;
            }
            i += 5;
        } else if (op == 0xCD && i + 2 <= hdr->code_size) {
            // int imm8
            i += 2;
        } else if ((op == 0xE9 || op == 0xE8) && i + 5 <= hdr->code_size) {
            // jmp/call rel32
            i += 5;
        } else if (op == 0x0F && i + 6 <= hdr->code_size) {
            // 2-byte conditional jump rel32
            i += 6;
        } else {
            // Default advance by 1 to resynchronize
            i += 1;
        }
    }
    if (reloc_count > 0) {
        printf("[run] Applied %d relocation(s) for data references.\n", reloc_count);
    }
    
    // Create process for isolation
    process_t* proc = create_process(filename, hdr->code_size, hdr->entry_point);
    if (!proc) {
        printf("[run] Failed to create process\n");
        free(buf);
        return;
    }
    
    printf("[run] DEBUG: Process created - code_start=%d, code_size=%d\n", (int)proc->code_start, (int)proc->code_size);
    printf("[run] DEBUG: Process pointer: %p, active: %d\n", proc, proc->active);
    printf("[run] DEBUG: USER_CODE_ADDR: %d\n", USER_CODE_ADDR);
    
    // Set up execution environment with process isolation
    
    // Copy code to user space
    if (!validate_user_memory_access(proc->code_start, hdr->code_size)) {
        printf("[run] Memory validation failed\n");
        free(buf);
        return;
    }
            memcpy((void*)proc->code_start, (char*)code, hdr->code_size);
    
    // Set up data section if present
    if (hdr->data_size > 0) {
        const uint8_t* data = (const uint8_t*)(buf + sizeof(struct eyn_exe_header) + hdr->code_size);
        uint32 data_addr = proc->code_start + 0x1000;
        printf("[run] Data section: %d bytes\n", (int)hdr->data_size);
        printf("[run] DEBUG: Source data pointer: %p\n", data);
        printf("[run] DEBUG: Target data address: %d (0x%X)\n", data_addr, data_addr);
        printf("[run] DEBUG: First few bytes of source data:\n");
        for (int i = 0; i < hdr->data_size && i < 20; i++) {
            printf("[run] DEBUG: data[%d] = 0x%02X ('%c')\n", i, (unsigned char)data[i], (data[i] >= 32 && data[i] <= 126) ? data[i] : '?');
        }
        if (validate_user_memory_access(data_addr, hdr->data_size)) {
            memcpy((void*)data_addr, (char*)data, hdr->data_size);
            printf("[run] DEBUG: Data copy completed\n");
        } else {
            printf("[run] DEBUG: Data memory validation failed\n");
        }
    }
    
    // Set up user stack
    // Stack is automatically set up by create_process
    
    // Simple instruction interpreter for user code
    printf("[run] Executing user code with instruction interpreter...\n");
    
    uint32_t pc = 0;
    uint32_t regs[8] = {0}; // eax, ecx, edx, ebx, esp, ebp, esi, edi
    
    while (pc < hdr->code_size) {
        uint8_t opcode = code[pc];
        
        if (opcode == 0xB8 || opcode == 0xB9 || opcode == 0xBA || opcode == 0xBB || 
            opcode == 0xBC || opcode == 0xBD || opcode == 0xBE || opcode == 0xBF) {
            // mov reg, imm32
            uint8_t reg = opcode - 0xB8;
            uint32_t imm = *(uint32_t*)(code + pc + 1);
            printf("[run] mov reg %d, %d\n", reg, imm);
            regs[reg] = imm;
            printf("[run] DEBUG: After mov - regs[%d] = %d\n", reg, regs[reg]);
            pc += 5;
        } else if (opcode == 0xCD) {
            // int imm8
            uint8_t imm = code[pc + 1];
            printf("[run] Executing syscall: eax=%d, ebx=%d, ecx=%d, edx=%d\n", 
                   regs[0], regs[3], regs[1], regs[2]);
            
            if (imm == 0x80) {
                // Handle syscall directly in the instruction interpreter
                if (regs[0] == 1) { // WRITE
                    printf("[run] WRITE: ");
                    printf("[run] DEBUG: Validating - fd=%d, addr=%d, len=%d\n", regs[3], regs[1], regs[2]);
                    printf("[run] DEBUG: Expected data range: %d to %d\n", (int)proc->code_start + 0x1000, (int)proc->code_start + 0x1000 + 100);
                    if (regs[3] == 1 && regs[1] >= (uint32_t)(proc->code_start + 0x1000) && regs[1] < (uint32_t)(proc->code_start + 0x1000 + 100) && regs[2] > 0 && regs[2] <= 100) {
                        // stdout, valid address range (data section), reasonable length
                        // Read string from user memory and print it directly
                        printf("[run] DEBUG: Validation passed, reading string from address %d\n", regs[1]);
                        char* buffer = (char*)regs[1];
                        printf("[run] DEBUG: Buffer pointer: %p\n", buffer);
                        printf("[run] DEBUG: Reading %d characters:\n", regs[2]);
                        for (int i = 0; i < regs[2] && i < 100; i++) {
                            char c = buffer[i];
                            printf("[run] DEBUG: buffer[%d] = 0x%02X ('%c')\n", i, (unsigned char)c, (c >= 32 && c <= 126) ? c : '?');
                            printf("%c", c);
                        }
                        printf("\n");
                        regs[0] = regs[2]; // Return bytes written
                    } else {
                        printf("Invalid write parameters - fd=%d, addr=%d, len=%d\n", regs[3], regs[1], regs[2]);
                        printf("[run] DEBUG: Validation failed - fd==1: %s, addr>=data_start: %s, addr<data_end: %s, len>0: %s, len<=100: %s\n", 
                               regs[3] == 1 ? "true" : "false",
                               regs[1] >= (uint32_t)(proc->code_start + 0x1000) ? "true" : "false",
                               regs[1] < (uint32_t)(proc->code_start + 0x1000 + 100) ? "true" : "false",
                               regs[2] > 0 ? "true" : "false",
                               regs[2] <= 100 ? "true" : "false");
                        regs[0] = -1; // Return error
                    }
                } else if (regs[0] == 2) { // EXIT
                    printf("[run] EXIT: %d\n", regs[3]);
                    break;
                }
            }
            pc += 2;
        } else if (opcode == 0xC3) {
            // ret
            printf("[run] ret instruction\n");
            break;
        } else {
            // Unknown opcode
            printf("[run] Unknown opcode: 0x%02X\n", opcode);
            pc++;
        }
    }
    
    program_exit:
    printf("[run] Program execution completed\n");
    
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
    
    free(buf);
}

// Process management functions for shell commands
int get_process_count() {
    int count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (g_processes[i].active) count++;
    }
    return count;
}

process_t* get_process_by_id(uint32 pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (g_processes[i].active && g_processes[i].pid == pid) {
            return &g_processes[i];
        }
    }
    return NULL;
}

int get_process_isolation_status() {
    return process_isolation_enabled;
}

REGISTER_SHELL_COMMAND(run, "run", run_cmd, CMD_STREAMING, "Run a .eyn executable with process isolation.\nUsage: run <program.eyn>", "run test.eyn"); 
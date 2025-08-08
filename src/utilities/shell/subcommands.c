#include <subcommands.h>
#include <eynfs.h>
#include <rei.h>
#include <string.h>
#include <util.h>
#include <vga.h>
#include <system.h>
#include <fs_commands.h>
#include <stdint.h>

#define EYNFS_SUPERBLOCK_LBA 2048
extern uint8_t g_current_drive;

// ============================================================================
// SEARCH SUB-COMMANDS
// ============================================================================

// Search criteria structure
typedef struct {
    int has_min;
    int has_max;
    int has_exact;
    uint32_t min_size;
    uint32_t max_size;
    uint32_t exact_size;
} search_size_criteria_t;

// Recursive size search function
void search_size_recursive(uint8 drive, const eynfs_superblock_t* sb, uint32_t dir_block, 
    char* current_path, int depth, int max_depth, search_size_criteria_t* criteria) {
    
    eynfs_dir_entry_t entries[EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t)];
    int count = eynfs_read_dir_table(drive, dir_block, entries, EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t));
    
    if (count < 0 || depth > max_depth) return;
    
    for (int i = 0; i < count; ++i) {
        if (entries[i].name[0] == '\0') continue;
        
        // Check if file matches size criteria
        if (entries[i].type == EYNFS_TYPE_FILE) {
            int matches = 0;
            
            if (criteria->has_exact) {
                matches = (entries[i].size == criteria->exact_size);
            } else {
                matches = 1;
                if (criteria->has_min && entries[i].size < criteria->min_size) matches = 0;
                if (criteria->has_max && entries[i].size > criteria->max_size) matches = 0;
            }
            
            if (matches) {
                printf("%c%s (%d bytes)\n", 255, 255, 255, entries[i].name, entries[i].size);
            }
        }
        
        // Recursively search subdirectories
        if (entries[i].type == EYNFS_TYPE_DIR && depth < max_depth) {
            char sub_path[256];
            if (strcmp(current_path, "/") == 0) {
                snprintf(sub_path, sizeof(sub_path), "/%s", entries[i].name);
            } else {
                snprintf(sub_path, sizeof(sub_path), "%s/%s", current_path, entries[i].name);
            }
            search_size_recursive(drive, sb, entries[i].first_block, sub_path, depth + 1, max_depth, criteria);
        }
    }
}

// Main search_size command implementation
void search_size_cmd(string ch) {
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    
    if (!ch[i]) {
        printf("%cUsage: search_size <operator> <size>\n", 255, 255, 255);
        printf("%cOperators: >, <, =, >=, <=, gt, lt, eq, gte, lte\n", 255, 255, 255);
        printf("%cExample: search_size > 1000\n", 255, 255, 255);
        printf("%cExample: search_size <= 50\n", 255, 255, 255);
        return;
    }
    
    // Parse operator
    char operator[8] = {0};
    uint8 j = 0;
    while (ch[i] && ch[i] != ' ' && j < 7) {
        operator[j++] = ch[i++];
    }
    operator[j] = '\0';
    
    while (ch[i] && ch[i] == ' ') i++;
    
    if (!ch[i]) {
        printf("%cError: Missing size value\n", 255, 0, 0);
        return;
    }
    
    // Parse size value
    uint32_t size_value = 0;
    while (ch[i] >= '0' && ch[i] <= '9') {
        size_value = size_value * 10 + (ch[i] - '0');
        i++;
    }
    
    // Set up search criteria
    search_size_criteria_t criteria = {0};
    
    if (strcmp(operator, ">") == 0 || strcmp(operator, "gt") == 0) {
        criteria.min_size = size_value + 1;
        criteria.has_min = 1;
    } else if (strcmp(operator, "<") == 0 || strcmp(operator, "lt") == 0) {
        criteria.max_size = size_value - 1;
        criteria.has_max = 1;
    } else if (strcmp(operator, "=") == 0 || strcmp(operator, "eq") == 0) {
        criteria.exact_size = size_value;
        criteria.has_exact = 1;
    } else if (strcmp(operator, ">=") == 0 || strcmp(operator, "gte") == 0) {
        criteria.min_size = size_value;
        criteria.has_min = 1;
    } else if (strcmp(operator, "<=") == 0 || strcmp(operator, "lte") == 0) {
        criteria.max_size = size_value;
        criteria.has_max = 1;
    } else {
        printf("%cError: Invalid operator '%s'\n", 255, 0, 0, operator);
        return;
    }
    
    printf("%cSearching for files %s %d bytes...\n", 255, 255, 255, operator, size_value);
    
    // Get filesystem info
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(g_current_drive, EYNFS_SUPERBLOCK_LBA, &sb) != 0 || sb.magic != EYNFS_MAGIC) {
        printf("%cError: No supported filesystem found.\n", 255, 0, 0);
        return;
    }
    
    search_size_recursive(g_current_drive, &sb, sb.root_dir_block, "/", 0, 10, &criteria);
}

// Recursive type search function
void search_type_recursive(uint8 drive, const eynfs_superblock_t* sb, uint32_t dir_block, 
                          char* current_path, int depth, int max_depth, const char* extension) {
    
    eynfs_dir_entry_t entries[EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t)];
    int count = eynfs_read_dir_table(drive, dir_block, entries, EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t));
    
    if (count < 0 || depth > max_depth) return;
    
    for (int i = 0; i < count; ++i) {
        if (entries[i].name[0] == '\0') continue;
        
        // Check if file has the specified extension
        if (entries[i].type == EYNFS_TYPE_FILE) {
            int name_len = strlen(entries[i].name);
            int ext_len = strlen(extension);
            
            if (name_len >= ext_len && strcmp(entries[i].name + name_len - ext_len, extension) == 0) {
                printf("%c%s\n", 255, 255, 255, entries[i].name);
            }
        }
        
        // Recursively search subdirectories
        if (entries[i].type == EYNFS_TYPE_DIR && depth < max_depth) {
            char sub_path[256];
            if (strcmp(current_path, "/") == 0) {
                snprintf(sub_path, sizeof(sub_path), "/%s", entries[i].name);
            } else {
                snprintf(sub_path, sizeof(sub_path), "%s/%s", current_path, entries[i].name);
            }
            search_type_recursive(drive, sb, entries[i].first_block, sub_path, depth + 1, max_depth, extension);
        }
    }
}

// Main search_type command implementation
void search_type_cmd(string ch) {
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    
    if (!ch[i]) {
        printf("%cUsage: search_type <extension>\n", 255, 255, 255);
        printf("%cExample: search_type .txt\n", 255, 255, 255);
        printf("%cExample: search_type .game\n", 255, 255, 255);
        return;
    }
    
    // Parse extension
    char extension[16] = {0};
    uint8 j = 0;
    while (ch[i] && ch[i] != ' ' && j < 15) {
        extension[j++] = ch[i++];
    }
    extension[j] = '\0';
    
    printf("%cSearching for files with extension '%s'...\n", 255, 255, 255, extension);
    
    // Get filesystem info
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(g_current_drive, EYNFS_SUPERBLOCK_LBA, &sb) != 0 || sb.magic != EYNFS_MAGIC) {
        printf("%cError: No supported filesystem found.\n", 255, 0, 0);
        return;
    }
    
    search_type_recursive(g_current_drive, &sb, sb.root_dir_block, "/", 0, 10, extension);
}

// Recursive empty search function
void search_empty_recursive(uint8 drive, const eynfs_superblock_t* sb, uint32_t dir_block, 
                           char* current_path, int depth, int max_depth) {
    
    eynfs_dir_entry_t entries[EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t)];
    int count = eynfs_read_dir_table(drive, dir_block, entries, EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t));
    
    if (count < 0 || depth > max_depth) return;
    
    for (int i = 0; i < count; ++i) {
        if (entries[i].name[0] == '\0') continue;
        
        if (entries[i].type == EYNFS_TYPE_FILE) {
            // Check for empty files
            if (entries[i].size == 0) {
                printf("%c%s (empty file)\n", 255, 255, 255, entries[i].name);
            }
        } else if (entries[i].type == EYNFS_TYPE_DIR) {
            // Check for empty directories
            eynfs_dir_entry_t sub_entries[EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t)];
            int sub_count = eynfs_read_dir_table(drive, entries[i].first_block, sub_entries, EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t));
            
            if (sub_count >= 0) {
                int has_entries = 0;
                for (int j = 0; j < sub_count; j++) {
                    if (sub_entries[j].name[0] != '\0') {
                        has_entries = 1;
                        break;
                    }
                }
                
                if (!has_entries) {
                    printf("%c%s/ (empty directory)\n", 120, 120, 255, entries[i].name);
                }
            }
            
            // Recursively search subdirectories
            if (depth < max_depth) {
                char sub_path[256];
                if (strcmp(current_path, "/") == 0) {
                    snprintf(sub_path, sizeof(sub_path), "/%s", entries[i].name);
                } else {
                    snprintf(sub_path, sizeof(sub_path), "%s/%s", current_path, entries[i].name);
                }
                search_empty_recursive(drive, sb, entries[i].first_block, sub_path, depth + 1, max_depth);
            }
        }
    }
}

// Main search_empty command implementation
void search_empty_cmd(string ch) {
    printf("%cSearching for empty files and directories...\n", 255, 255, 255);
    
    // Get filesystem info
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(g_current_drive, EYNFS_SUPERBLOCK_LBA, &sb) != 0 || sb.magic != EYNFS_MAGIC) {
        printf("%cError: No supported filesystem found.\n", 255, 0, 0);
        return;
    }
    
    search_empty_recursive(g_current_drive, &sb, sb.root_dir_block, "/", 0, 10);
}

// Recursive depth-limited search function
void search_depth_recursive(uint8 drive, const eynfs_superblock_t* sb, uint32_t dir_block, 
                           char* current_path, int depth, int max_depth, const char* pattern) {
    
    eynfs_dir_entry_t entries[EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t)];
    int count = eynfs_read_dir_table(drive, dir_block, entries, EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t));
    
    if (count < 0 || depth > max_depth) return;
    
    for (int i = 0; i < count; ++i) {
        if (entries[i].name[0] == '\0') continue;
        
        // Check if name matches pattern
        if (strstr(entries[i].name, pattern) != NULL) {
            printf("%c%s (%s)\n", 255, 255, 255, entries[i].name, 
                   entries[i].type == EYNFS_TYPE_DIR ? "DIR" : "FILE");
        }
        
        // Recursively search subdirectories
        if (entries[i].type == EYNFS_TYPE_DIR && depth < max_depth) {
            char sub_path[256];
            if (strcmp(current_path, "/") == 0) {
                snprintf(sub_path, sizeof(sub_path), "/%s", entries[i].name);
            } else {
                snprintf(sub_path, sizeof(sub_path), "%s/%s", current_path, entries[i].name);
            }
            search_depth_recursive(drive, sb, entries[i].first_block, sub_path, depth + 1, max_depth, pattern);
        }
    }
}

// Main search_depth command implementation
void search_depth_cmd(string ch) {
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    
    if (!ch[i]) {
        printf("%cUsage: search_depth <max_depth> <pattern>\n", 255, 255, 255);
        printf("%cExample: search_depth 2 hello\n", 255, 255, 255);
        printf("%cExample: search_depth 3 .txt\n", 255, 255, 255);
        return;
    }
    
    // Parse max depth
    int max_depth = 0;
    while (ch[i] >= '0' && ch[i] <= '9') {
        max_depth = max_depth * 10 + (ch[i] - '0');
        i++;
    }
    
    if (max_depth <= 0 || max_depth > 10) {
        printf("%cError: Invalid depth (1-10)\n", 255, 0, 0);
        return;
    }
    
    while (ch[i] && ch[i] == ' ') i++;
    
    if (!ch[i]) {
        printf("%cError: Missing search pattern\n", 255, 0, 0);
        return;
    }
    
    // Parse pattern
    char pattern[64] = {0};
    uint8 j = 0;
    while (ch[i] && ch[i] != ' ' && j < 63) {
        pattern[j++] = ch[i++];
    }
    pattern[j] = '\0';
    
    printf("%cSearching for '%s' within depth %d...\n", 255, 255, 255, pattern, max_depth);
    
    // Get filesystem info
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(g_current_drive, EYNFS_SUPERBLOCK_LBA, &sb) != 0 || sb.magic != EYNFS_MAGIC) {
        printf("%cError: No supported filesystem found.\n", 255, 0, 0);
        return;
    }
    
    search_depth_recursive(g_current_drive, &sb, sb.root_dir_block, "/", 0, max_depth, pattern);
} 

// ============================================================================
// LS SUB-COMMANDS
// ============================================================================

// Tree view for ls
void ls_tree_recursive(uint8 drive, const eynfs_superblock_t* sb, uint32_t dir_block, 
                       char* current_path, int depth, int max_depth, int indent) {
    
    eynfs_dir_entry_t entries[EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t)];
    int count = eynfs_read_dir_table(drive, dir_block, entries, EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t));
    
    if (count < 0 || depth > max_depth) return;
    
    for (int i = 0; i < count; ++i) {
        if (entries[i].name[0] == '\0') continue;
        
        // Print indentation
        for (int d = 0; d < indent; ++d) {
            printf("%c  ", 120, 120, 255);
        }
        
        // Print tree connector
        if (indent > 0) {
            printf("%c├─ ", 120, 120, 255);
        }
        
        // Print entry name with color coding
        if (entries[i].type == EYNFS_TYPE_DIR) {
            printf("%c%s/\n", 120, 120, 255, entries[i].name);
            // Recursively list subdirectories
            if (depth < max_depth) {
                char sub_path[256];
                if (strcmp(current_path, "/") == 0) {
                    snprintf(sub_path, sizeof(sub_path), "/%s", entries[i].name);
                } else {
                    snprintf(sub_path, sizeof(sub_path), "%s/%s", current_path, entries[i].name);
                }
                ls_tree_recursive(drive, sb, entries[i].first_block, sub_path, depth + 1, max_depth, indent + 1);
            }
        } else {
            printf("%c%s\n", 255, 255, 255, entries[i].name);
        }
    }
}

// Main ls_tree command implementation
void ls_tree_cmd(string ch) {
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    
    int max_depth = 3; // Default depth
    if (ch[i]) {
        max_depth = 0;
        while (ch[i] >= '0' && ch[i] <= '9') {
            max_depth = max_depth * 10 + (ch[i] - '0');
            i++;
        }
        if (max_depth <= 0) max_depth = 3;
        if (max_depth > 10) max_depth = 10; // Limit depth
    }
    
    printf("%cDirectory tree (max depth: %d):\n", 255, 255, 255, max_depth);
    printf("%c/\n", 120, 120, 255);
    
    // Get filesystem info
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(g_current_drive, EYNFS_SUPERBLOCK_LBA, &sb) != 0 || sb.magic != EYNFS_MAGIC) {
        printf("%cError: No supported filesystem found.\n", 255, 0, 0);
        return;
    }
    
    ls_tree_recursive(g_current_drive, &sb, sb.root_dir_block, "/", 0, max_depth, 0);
}

// Size-based ls with file sizes
void ls_size_recursive(uint8 drive, const eynfs_superblock_t* sb, uint32_t dir_block, 
                       char* current_path, int depth, int max_depth) {
    
    eynfs_dir_entry_t entries[EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t)];
    int count = eynfs_read_dir_table(drive, dir_block, entries, EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t));
    
    if (count < 0 || depth > max_depth) return;
    
    for (int i = 0; i < count; ++i) {
        if (entries[i].name[0] == '\0') continue;
        
        // Print indentation
        for (int d = 0; d < depth; ++d) {
            printf("%c  ", 120, 120, 255);
        }
        
        // Print entry with size
        if (entries[i].type == EYNFS_TYPE_DIR) {
            printf("%c%s/ [DIR]\n", 120, 120, 255, entries[i].name);
            // Recursively list subdirectories
            if (depth < max_depth) {
                char sub_path[256];
                if (strcmp(current_path, "/") == 0) {
                    snprintf(sub_path, sizeof(sub_path), "/%s", entries[i].name);
                } else {
                    snprintf(sub_path, sizeof(sub_path), "%s/%s", current_path, entries[i].name);
                }
                ls_size_recursive(drive, sb, entries[i].first_block, sub_path, depth + 1, max_depth);
            }
        } else {
            // Format size nicely
            char size_str[16];
            if (entries[i].size < 1024) {
                snprintf(size_str, sizeof(size_str), "%d B", entries[i].size);
            } else if (entries[i].size < 1024 * 1024) {
                snprintf(size_str, sizeof(size_str), "%.1f KB", entries[i].size / 1024.0);
            } else {
                snprintf(size_str, sizeof(size_str), "%.1f MB", entries[i].size / (1024.0 * 1024.0));
            }
            printf("%c%s [%s]\n", 255, 255, 255, entries[i].name, size_str);
        }
    }
}

// Main ls_size command implementation
void ls_size_cmd(string ch) {
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    
    int max_depth = 1; // Default depth
    if (ch[i]) {
        max_depth = 0;
        while (ch[i] >= '0' && ch[i] <= '9') {
            max_depth = max_depth * 10 + (ch[i] - '0');
            i++;
        }
        if (max_depth <= 0) max_depth = 1;
        if (max_depth > 5) max_depth = 5; // Limit depth
    }
    
    printf("%cDirectory listing with sizes (depth: %d):\n", 255, 255, 255, max_depth);
    
    // Get filesystem info
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(g_current_drive, EYNFS_SUPERBLOCK_LBA, &sb) != 0 || sb.magic != EYNFS_MAGIC) {
        printf("%cError: No supported filesystem found.\n", 255, 0, 0);
        return;
    }
    
    ls_size_recursive(g_current_drive, &sb, sb.root_dir_block, "/", 0, max_depth);
}

// Detailed ls with more information
void ls_detail_recursive(uint8 drive, const eynfs_superblock_t* sb, uint32_t dir_block, 
                         char* current_path, int depth, int max_depth) {
    
    eynfs_dir_entry_t entries[EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t)];
    int count = eynfs_read_dir_table(drive, dir_block, entries, EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t));
    
    if (count < 0 || depth > max_depth) return;
    
    for (int i = 0; i < count; ++i) {
        if (entries[i].name[0] == '\0') continue;
        
        // Print indentation
        for (int d = 0; d < depth; ++d) {
            printf("%c  ", 120, 120, 255);
        }
        
        // Print detailed entry information
        if (entries[i].type == EYNFS_TYPE_DIR) {
            printf("%c[DIR] %s/ (block: %d)\n", 120, 120, 255, entries[i].name, entries[i].first_block);
        } else {
            // Format size nicely
            char size_str[16];
            if (entries[i].size < 1024) {
                snprintf(size_str, sizeof(size_str), "%d B", entries[i].size);
            } else if (entries[i].size < 1024 * 1024) {
                snprintf(size_str, sizeof(size_str), "%.1f KB", entries[i].size / 1024.0);
            } else {
                snprintf(size_str, sizeof(size_str), "%.1f MB", entries[i].size / (1024.0 * 1024.0));
            }
            printf("%c[FILE] %s (size: %s, block: %d)\n", 255, 255, 255, entries[i].name, size_str, entries[i].first_block);
        }
        
        // Recursively list subdirectories
        if (entries[i].type == EYNFS_TYPE_DIR && depth < max_depth) {
            char sub_path[256];
            if (strcmp(current_path, "/") == 0) {
                snprintf(sub_path, sizeof(sub_path), "/%s", entries[i].name);
            } else {
                snprintf(sub_path, sizeof(sub_path), "%s/%s", current_path, entries[i].name);
            }
            ls_detail_recursive(drive, sb, entries[i].first_block, sub_path, depth + 1, max_depth);
        }
    }
}

// Main ls_detail command implementation
void ls_detail_cmd(string ch) {
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    
    int max_depth = 1; // Default depth
    if (ch[i]) {
        max_depth = 0;
        while (ch[i] >= '0' && ch[i] <= '9') {
            max_depth = max_depth * 10 + (ch[i] - '0');
            i++;
        }
        if (max_depth <= 0) max_depth = 1;
        if (max_depth > 3) max_depth = 3; // Limit depth for detail view
    }
    
    printf("%cDetailed directory listing (depth: %d):\n", 255, 255, 255, max_depth);
    printf("%cFormat: [TYPE] name (size, block)\n", 255, 255, 255);
    
    // Get filesystem info
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(g_current_drive, EYNFS_SUPERBLOCK_LBA, &sb) != 0 || sb.magic != EYNFS_MAGIC) {
        printf("%cError: No supported filesystem found.\n", 255, 0, 0);
        return;
    }
    
    ls_detail_recursive(g_current_drive, &sb, sb.root_dir_block, "/", 0, max_depth);
}

// ============================================================================
// FILESYSTEM UTILITY SUB-COMMANDS
// ============================================================================

// Filesystem status command
void fsstat_cmd(string ch) {
    printf("%c=== Filesystem Status ===\n", 255, 255, 255);
    
    // Get filesystem info
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(g_current_drive, EYNFS_SUPERBLOCK_LBA, &sb) != 0 || sb.magic != EYNFS_MAGIC) {
        printf("%cError: No supported filesystem found.\n", 255, 0, 0);
        return;
    }
    
    printf("%cFilesystem: EYNFS v%d\n", 255, 255, 255, sb.version);
    printf("%cTotal blocks: %d\n", 255, 255, 255, sb.total_blocks);
    printf("%cRoot directory block: %d\n", 255, 255, 255, sb.root_dir_block);
    printf("%cBlock size: %d bytes\n", 255, 255, 255, EYNFS_BLOCK_SIZE);
    printf("%cTotal capacity: %.2f MB\n", 255, 255, 255, 
           (sb.total_blocks * EYNFS_BLOCK_SIZE) / (1024.0 * 1024.0));
    
    // Calculate free blocks
    int free_blocks = 0;
    for (int i = 0; i < sb.total_blocks; i++) {
        uint8_t bitmap_byte = 0;
        ata_read_sector(g_current_drive, EYNFS_SUPERBLOCK_LBA + 1 + (i / 8), &bitmap_byte);
        if (!(bitmap_byte & (1 << (i % 8)))) {
            free_blocks++;
        }
    }
    
    printf("%cFree blocks: %d\n", 255, 255, 255, free_blocks);
    printf("%cUsed blocks: %d\n", 255, 255, 255, sb.total_blocks - free_blocks);
    printf("%cFree space: %.2f MB\n", 255, 255, 255, 
           (free_blocks * EYNFS_BLOCK_SIZE) / (1024.0 * 1024.0));
    printf("%cUsage: %.1f%%\n", 255, 255, 255, 
           ((sb.total_blocks - free_blocks) * 100.0) / sb.total_blocks);
}

// Cache statistics command
void cache_stats_cmd(string ch) {
    printf("%c=== Cache Statistics ===\n", 255, 255, 255);
    
    // Get cache statistics from EYNFS
    uint32_t cache_hits, cache_misses;
    eynfs_get_cache_stats(&cache_hits, &cache_misses);
    
    printf("%cBlock cache hits: %d\n", 255, 255, 255, cache_hits);
    printf("%cBlock cache misses: %d\n", 255, 255, 255, cache_misses);
    
    if (cache_hits + cache_misses > 0) {
        float hit_rate = (cache_hits * 100.0) / (cache_hits + cache_misses);
        printf("%cCache hit rate: %.1f%%\n", 255, 255, 255, hit_rate);
    }
    
    printf("%c\n", 255, 255, 255);
    printf("%cUse 'cache_clear' to clear all caches\n", 255, 255, 255);
    printf("%cUse 'cache_reset' to reset statistics\n", 255, 255, 255);
}

// Clear cache command
void cache_clear_cmd(string ch) {
    printf("%cClearing all caches...\n", 255, 255, 255);
    
    eynfs_cache_clear();
    
    printf("%cAll caches cleared successfully.\n", 0, 255, 0);
}

// Reset cache statistics command
void cache_reset_cmd(string ch) {
    printf("%cResetting cache statistics...\n", 255, 255, 255);
    
    eynfs_reset_cache_stats();
    
    printf("%cCache statistics reset successfully.\n", 0, 255, 0);
}

// Block usage visualization command
void blockmap_cmd(string ch) {
    printf("%c=== Block Usage Map ===\n", 255, 255, 255);
    
    // Get filesystem info
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(g_current_drive, EYNFS_SUPERBLOCK_LBA, &sb) != 0 || sb.magic != EYNFS_MAGIC) {
        printf("%cError: No supported filesystem found.\n", 255, 0, 0);
        return;
    }
    
    printf("%cBlock map (0=free, 1=used):\n", 255, 255, 255);
    printf("%c", 255, 255, 255);
    
    // Display block map in a grid format
    int blocks_per_line = 64;
    for (int i = 0; i < sb.total_blocks; i++) {
        if (i % blocks_per_line == 0 && i > 0) {
            printf("\n%c", 255, 255, 255);
        }
        
        // Read bitmap
        uint8_t bitmap_byte = 0;
        ata_read_sector(g_current_drive, EYNFS_SUPERBLOCK_LBA + 1 + (i / 8), &bitmap_byte);
        
        if (bitmap_byte & (1 << (i % 8))) {
            printf("%c1", 255, 0, 0); // Red for used
        } else {
            printf("%c0", 0, 255, 0); // Green for free
        }
        
        // Add spacing every 8 blocks
        if ((i + 1) % 8 == 0) {
            printf(" ");
        }
    }
    
    printf("\n%cLegend: 0=Free (green), 1=Used (red)\n", 255, 255, 255);
    printf("%cTotal blocks: %d\n", 255, 255, 255, sb.total_blocks);
}

// Debug superblock command
void debug_superblock_cmd(string ch) {
    printf("%c=== Superblock Debug Info ===\n", 255, 255, 255);
    
    // Get filesystem info
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(g_current_drive, EYNFS_SUPERBLOCK_LBA, &sb) != 0 || sb.magic != EYNFS_MAGIC) {
        printf("%cError: No supported filesystem found.\n", 255, 0, 0);
        return;
    }
    
    printf("%cMagic number: 0x%08X\n", 255, 255, 255, sb.magic);
    printf("%cVersion: %d\n", 255, 255, 255, sb.version);
    printf("%cTotal blocks: %d\n", 255, 255, 255, sb.total_blocks);
    printf("%cRoot directory block: %d\n", 255, 255, 255, sb.root_dir_block);
    printf("%cFree block map starts at: LBA %d\n", 255, 255, 255, EYNFS_SUPERBLOCK_LBA + 1);
    printf("%cBlock size: %d bytes\n", 255, 255, 255, EYNFS_BLOCK_SIZE);
    printf("%cSuperblock LBA: %d\n", 255, 255, 255, EYNFS_SUPERBLOCK_LBA);
    
    // Show first few bytes of bitmap
    printf("%c\n", 255, 255, 255);
    printf("%cFirst 32 bytes of free block bitmap:\n", 255, 255, 255);
    printf("%c", 255, 255, 255);
    
    for (int i = 0; i < 32; i++) {
        uint8_t bitmap_byte = 0;
        ata_read_sector(g_current_drive, EYNFS_SUPERBLOCK_LBA + 1 + i, &bitmap_byte);
        printf("%02X ", bitmap_byte);
        if ((i + 1) % 16 == 0) {
            printf("\n%c", 255, 255, 255);
        }
    }
    printf("\n");
}

// Debug directory structure command
void debug_directory_cmd(string ch) {
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    
    if (!ch[i]) {
        printf("%cUsage: debug_directory <path>\n", 255, 255, 255);
        printf("%cExample: debug_directory /\n", 255, 255, 255);
        printf("%cExample: debug_directory /games\n", 255, 255, 255);
        return;
    }
    
    // Parse path
    char path[128] = {0};
    uint8 j = 0;
    while (ch[i] && ch[i] != ' ' && j < 127) {
        path[j++] = ch[i++];
    }
    path[j] = '\0';
    
    printf("%c=== Directory Debug: %s ===\n", 255, 255, 255, path);
    
    // Get filesystem info
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(g_current_drive, EYNFS_SUPERBLOCK_LBA, &sb) != 0 || sb.magic != EYNFS_MAGIC) {
        printf("%cError: No supported filesystem found.\n", 255, 0, 0);
        return;
    }
    
    // Find directory
    eynfs_dir_entry_t entry;
    uint32_t parent_block, entry_idx;
    if (eynfs_traverse_path(g_current_drive, &sb, path, &entry, &parent_block, &entry_idx) != 0) {
        printf("%cError: Directory not found.\n", 255, 0, 0);
        return;
    }
    
    if (entry.type != EYNFS_TYPE_DIR) {
        printf("%cError: Path is not a directory.\n", 255, 0, 0);
        return;
    }
    
    printf("%cDirectory block: %d\n", 255, 255, 255, entry.first_block);
    printf("%cDirectory size: %d bytes\n", 255, 255, 255, entry.size);
    printf("%c\n", 255, 255, 255);
    
    // Read and display directory entries
    eynfs_dir_entry_t entries[EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t)];
    int count = eynfs_read_dir_table(g_current_drive, entry.first_block, entries, EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t));
    
    if (count < 0) {
        printf("%cError: Failed to read directory.\n", 255, 0, 0);
        return;
    }
    
    printf("%cDirectory entries (%d total):\n", 255, 255, 255, count);
    for (int k = 0; k < count; k++) {
        if (entries[k].name[0] == '\0') {
            printf("%c  [%d] <empty>\n", 120, 120, 255, k);
        } else {
            printf("%c  [%d] %s (%s, block: %d, size: %d)\n", 255, 255, 255, k, entries[k].name,
                   entries[k].type == EYNFS_TYPE_DIR ? "DIR" : "FILE", 
                   entries[k].first_block, entries[k].size);
        }
    }
}

// ============================================================================
// HELP SUB-COMMANDS
// ============================================================================

// Help for write command
void help_write_cmd(string ch) {
    printf("%c=== Write Command Help ===\n", 255, 255, 255);
    printf("%cThe 'write' command opens a text editor for creating/editing files.\n", 255, 255, 255);
    printf("%c\n", 255, 255, 255);
    printf("%cUsage: write <filename>\n", 255, 255, 255);
    printf("%cExample: write myfile.txt\n", 255, 255, 255);
    printf("%c\n", 255, 255, 255);
    printf("%cEditor Controls:\n", 255, 255, 255);
    printf("%c  Ctrl+S: Save and exit\n", 255, 255, 255);
    printf("%c  Ctrl+Q: Exit without saving\n", 255, 255, 255);
    printf("%c  Arrow keys: Navigate\n", 255, 255, 255);
    printf("%c  Type normally to edit\n", 255, 255, 255);
    printf("%c\n", 255, 255, 255);
    printf("%cThe editor supports basic text editing with a simple interface.\n", 255, 255, 255);
}

// Help for search command and sub-commands
void help_search_cmd(string ch) {
    printf("%c=== Search Commands Help ===\n", 255, 255, 255);
    printf("%c\n", 255, 255, 255);
    printf("%cBasic Search:\n", 255, 255, 255);
    printf("%c  search <pattern> [options]  - Search for text pattern in files\n", 255, 255, 255);
    printf("%c\n", 255, 255, 255);
    printf("%cSearch Sub-commands:\n", 255, 255, 255);
    printf("%c  search_size <op> <size>     - Find files by size\n", 255, 255, 255);
    printf("%c  search_type <extension>     - Find files by extension\n", 255, 255, 255);
    printf("%c  search_empty                - Find empty files/directories\n", 255, 255, 255);
    printf("%c  search_depth <depth> <pat>  - Search with depth limit\n", 255, 255, 255);
    printf("%c\n", 255, 255, 255);
    printf("%cSize operators: >, <, =, >=, <=, gt, lt, eq, gte, lte\n", 255, 255, 255);
    printf("%cExamples:\n", 255, 255, 255);
    printf("%c  search_size > 1000\n", 255, 255, 255);
    printf("%c  search_type .txt\n", 255, 255, 255);
    printf("%c  search_depth 2 hello\n", 255, 255, 255);
}

// Help for filesystem commands
void help_fs_cmd(string ch) {
    printf("%c=== Filesystem Commands Help ===\n", 255, 255, 255);
    printf("%c\n", 255, 255, 255);
    printf("%cNavigation:\n", 255, 255, 255);
    printf("%c  ls [path]                   - List directory contents\n", 255, 255, 255);
    printf("%c  cd <path>                   - Change directory\n", 255, 255, 255);
    printf("%c\n", 255, 255, 255);
    printf("%cFile Operations:\n", 255, 255, 255);
    printf("%c  read <file>                 - Display file contents\n", 255, 255, 255);
    printf("%c  write <file>                - Edit/create file\n", 255, 255, 255);
    printf("%c  del <file>                  - Delete file\n", 255, 255, 255);
    printf("%c  copy <src> <dest>           - Copy file\n", 255, 255, 255);
    printf("%c  move <src> <dest>           - Move file\n", 255, 255, 255);
    printf("%c  size <file>                 - Show file size\n", 255, 255, 255);
    printf("%c\n", 255, 255, 255);
    printf("%cDirectory Operations:\n", 255, 255, 255);
    printf("%c  makedir <name>              - Create directory\n", 255, 255, 255);
    printf("%c  deldir <name>               - Delete directory\n", 255, 255, 255);
    printf("%c\n", 255, 255, 255);
    printf("%cDrive Management:\n", 255, 255, 255);
    printf("%c  drive <drive>               - Switch to drive\n", 255, 255, 255);
    printf("%c  drives                       - List available drives\n", 255, 255, 255);
}

// Help for editing commands
void help_edit_cmd(string ch) {
    printf("%c=== Editing Commands Help ===\n", 255, 255, 255);
    printf("%c\n", 255, 255, 255);
    printf("%cText Editor:\n", 255, 255, 255);
    printf("%c  write <filename>            - Open text editor\n", 255, 255, 255);
    printf("%c\n", 255, 255, 255);
    printf("%cEditor Controls:\n", 255, 255, 255);
    printf("%c  Ctrl+S                      - Save and exit\n", 255, 255, 255);
    printf("%c  Ctrl+Q                      - Exit without saving\n", 255, 255, 255);
    printf("%c  Arrow keys                  - Navigate cursor\n", 255, 255, 255);
    printf("%c  Backspace/Delete            - Delete characters\n", 255, 255, 255);
    printf("%c  Type normally               - Insert text\n", 255, 255, 255);
    printf("%c\n", 255, 255, 255);
    printf("%cFeatures:\n", 255, 255, 255);
    printf("%c  - Simple line-based editor\n", 255, 255, 255);
    printf("%c  - Auto-save on exit\n", 255, 255, 255);
    printf("%c  - File creation if doesn't exist\n", 255, 255, 255);
}

// Help for system commands
void help_system_cmd(string ch) {
    printf("%c=== System Commands Help ===\n", 255, 255, 255);
    printf("%c\n", 255, 255, 255);
    printf("%cSystem Info:\n", 255, 255, 255);
    printf("%c  ver                         - Show OS version\n", 255, 255, 255);
    printf("%c  clear                       - Clear screen\n", 255, 255, 255);
    printf("%c\n", 255, 255, 255);
    printf("%cUtilities:\n", 255, 255, 255);
    printf("%c  echo <text>                 - Print text\n", 255, 255, 255);
    printf("%c  calc <expression>           - Calculator\n", 255, 255, 255);
    printf("%c  random [max]                - Generate random number\n", 255, 255, 255);
    printf("%c  sort <file>                 - Sort file contents\n", 255, 255, 255);
    printf("%c\n", 255, 255, 255);
    printf("%cHistory:\n", 255, 255, 255);
    printf("%c  history                     - Show command history\n", 255, 255, 255);
    printf("%c  log                         - Show system log\n", 255, 255, 255);
    printf("%c\n", 255, 255, 255);
    printf("%cStorage:\n", 255, 255, 255);
    printf("%c  fdisk                       - Disk partitioning\n", 255, 255, 255);
    printf("%c  format <drive>              - Format drive\n", 255, 255, 255);
    printf("%c  lsata                       - List ATA devices\n", 255, 255, 255);
}

// Help for game commands
void help_game_cmd(string ch) {
    printf("%c=== Game Commands Help ===\n", 255, 255, 255);
    printf("%c\n", 255, 255, 255);
    printf("%cGame Engine:\n", 255, 255, 255);
    printf("%c  game <filename>             - Run game file\n", 255, 255, 255);
    printf("%c\n", 255, 255, 255);
    printf("%cGame Files:\n", 255, 255, 255);
    printf("%c  - Games are stored in .game format\n", 255, 255, 255);
    printf("%c  - Located in /games/ directory\n", 255, 255, 255);
    printf("%c  - Simple text-based games\n", 255, 255, 255);
    printf("%c\n", 255, 255, 255);
    printf("%cExamples:\n", 255, 255, 255);
    printf("%c  game snake                  - Run snake game\n", 255, 255, 255);
    printf("%c  game /games/snake.game      - Run specific game\n", 255, 255, 255);
}

// Help for development commands
void help_dev_cmd(string ch) {
    printf("%c=== Development Commands Help ===\n", 255, 255, 255);
    printf("%c\n", 255, 255, 255);
    printf("%cAssembly:\n", 255, 255, 255);
    printf("%c  assemble <file>             - Assemble assembly file\n", 255, 255, 255);
    printf("%c  run <file>                  - Run executable file\n", 255, 255, 255);
    printf("%c\n", 255, 255, 255);
    printf("%cAssembly Language:\n", 255, 255, 255);
    printf("%c  - Simple x86 assembly\n", 255, 255, 255);
    printf("%c  - Basic instructions supported\n", 255, 255, 255);
    printf("%c  - Outputs executable format\n", 255, 255, 255);
    printf("%c\n", 255, 255, 255);
    printf("%cExamples:\n", 255, 255, 255);
    printf("%c  assemble myprog.asm         - Assemble program\n", 255, 255, 255);
    printf("%c  run myprog.bin              - Run executable\n", 255, 255, 255);
} 

// ============================================================================
// READ SUB-COMMANDS
// ============================================================================

// read_raw command implementation - dynamic buffer system
void read_raw_cmd(string ch) {
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    
    if (!ch[i]) {
        printf("%cUsage: read_raw <filename>\n", 255, 255, 255);
        printf("%cDisplay raw file contents.\n", 255, 255, 255);
        return;
    }
    
    // Parse filename
    char arg[128] = {0};
    uint8 j = 0;
    while (ch[i] && ch[i] != ' ' && j < 127) {
        arg[j++] = ch[i++];
    }
    arg[j] = '\0';
    
    // Resolve path
    char abspath[128];
    resolve_path(arg, shell_current_path, abspath, sizeof(abspath));
    
    // Try EYNFS first
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(g_current_drive, EYNFS_SUPERBLOCK_LBA, &sb) == 0 && sb.magic == EYNFS_MAGIC) {
        eynfs_dir_entry_t entry;
        uint32_t parent_block, entry_idx;
        if (eynfs_traverse_path(g_current_drive, &sb, abspath, &entry, &parent_block, &entry_idx) == 0) {
            if (entry.type == EYNFS_TYPE_FILE) {
                // Dynamic buffer sizing based on file size
                uint32_t buffer_size;
                if (entry.size <= 16384) { // 16KB or smaller
                    buffer_size = entry.size + 1;
                } else if (entry.size <= 65536) { // 64KB or smaller
                    buffer_size = entry.size + 1;
                } else {
                    // For very large files, limit to 64KB
                    buffer_size = 65537; // 64KB + 1 for null terminator
                }
                
                char* buffer = (char*)my_malloc(buffer_size);
                if (buffer) {
                    // Read the file (up to buffer size)
                    int bytes_to_read = (entry.size < buffer_size - 1) ? entry.size : buffer_size - 1;
                    int bytes_read = eynfs_read_file(g_current_drive, &sb, &entry, buffer, bytes_to_read, 0);
                    
                    if (bytes_read > 0) {
                        buffer[bytes_read] = '\0'; // Null terminate
                        printf("%s", buffer); // Just print the content
                        printf("\n"); // Add newline after content
                        
                        // If we couldn't read the entire file, show a message
                        if (bytes_read < entry.size) {
                            printf("%c[File truncated - showing first %d bytes of %d total]\n", 255, 165, 0, bytes_read, entry.size);
                        }
                    } else {
                        printf("%cError: Failed to read file.\n", 255, 0, 0);
                    }
                    
                    my_free(buffer);
                } else {
                    printf("%cError: Memory allocation failed for buffer (%d bytes).\n", 255, 0, 0, buffer_size);
                }
            } else {
                printf("%cError: Path is not a file.\n", 255, 0, 0);
            }
        } else {
            printf("%cError: File not found.\n", 255, 0, 0);
        }
    } else {
        // FAT32 fallback
        printf("%cError: No supported filesystem found.\n", 255, 0, 0);
    }
}

// read_md command implementation - markdown rendering
void read_md_cmd(string ch) {
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    
    if (!ch[i]) {
        printf("%cUsage: read_md <filename>\n", 255, 255, 255);
        printf("%cDisplay markdown files with formatting.\n", 255, 255, 255);
        return;
    }
    
    // Parse filename
    char arg[128] = {0};
    uint8 j = 0;
    while (ch[i] && ch[i] != ' ' && j < 127) {
        arg[j++] = ch[i++];
    }
    arg[j] = '\0';
    
    // Check file extension
    int name_len = strlen(arg);
    int is_md = (name_len >= 3 && strcmp(arg + name_len - 3, ".md") == 0);
    
    if (!is_md) {
        printf("%cError: Only .md markdown files are supported.\n", 255, 0, 0);
        return;
    }
    
    // Resolve path
    char abspath[128];
    resolve_path(arg, shell_current_path, abspath, sizeof(abspath));
    
    // Try EYNFS first
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(g_current_drive, EYNFS_SUPERBLOCK_LBA, &sb) == 0 && sb.magic == EYNFS_MAGIC) {
        eynfs_dir_entry_t entry;
        uint32_t parent_block, entry_idx;
        if (eynfs_traverse_path(g_current_drive, &sb, abspath, &entry, &parent_block, &entry_idx) == 0) {
            if (entry.type == EYNFS_TYPE_FILE) {
                // Dynamic buffer sizing based on file size
                uint32_t buffer_size;
                if (entry.size <= 16384) { // 16KB or smaller
                    buffer_size = entry.size + 1;
                } else if (entry.size <= 65536) { // 64KB or smaller
                    buffer_size = entry.size + 1;
                } else {
                    // For very large files, limit to 64KB
                    buffer_size = 65537; // 64KB + 1 for null terminator
                }
                
                char* buffer = (char*)my_malloc(buffer_size);
                if (buffer) {
                    // Read the file (up to buffer size)
                    int bytes_to_read = (entry.size < buffer_size - 1) ? entry.size : buffer_size - 1;
                    int bytes_read = eynfs_read_file(g_current_drive, &sb, &entry, buffer, bytes_to_read, 0);
                    
                    if (bytes_read > 0) {
                        buffer[bytes_read] = '\0'; // Null terminate
                        
                        // Render the markdown content
                        printf("%cRendering markdown file: %s (%d bytes)\n", 0, 255, 0, arg, bytes_read);
                        render_markdown(buffer);
                        printf("\n"); // Add newline after rendering
                        
                        // If we couldn't read the entire file, show a message
                        if (bytes_read < entry.size) {
                            printf("%c[File truncated - showing first %d bytes of %d total]\n", 255, 165, 0, bytes_read, entry.size);
                        }
                    } else {
                        printf("%cError: Failed to read markdown file.\n", 255, 0, 0);
                    }
                    
                    my_free(buffer);
                } else {
                    printf("%cError: Memory allocation failed for buffer (%d bytes).\n", 255, 0, 0, buffer_size);
                }
            } else {
                printf("%cError: Path is not a file.\n", 255, 0, 0);
            }
        } else {
            printf("%cError: File not found.\n", 255, 0, 0);
        }
    } else {
        // FAT32 fallback
        printf("%cError: No supported filesystem found.\n", 255, 0, 0);
    }
}

// read_image command implementation - REI image rendering
void read_image_cmd(string ch) {
    uint8 i = 0;
    while (ch[i] && ch[i] != ' ') i++;
    while (ch[i] && ch[i] == ' ') i++;
    
    if (!ch[i]) {
        printf("%cUsage: read_image <filename>\n", 255, 255, 255);
        printf("%cDisplay image files (.rei format).\n", 255, 255, 255);
        return;
    }
    
    // Parse filename
    char arg[128] = {0};
    uint8 j = 0;
    while (ch[i] && ch[i] != ' ' && j < 127) {
        arg[j++] = ch[i++];
    }
    arg[j] = '\0';
    
    // Check file extension
    int name_len = strlen(arg);
    int is_rei = (name_len >= 4 && strcmp(arg + name_len - 4, ".rei") == 0);
    
    if (!is_rei) {
        printf("%cError: Only .rei image files are supported.\n", 255, 0, 0);
        return;
    }
    
    // Resolve path
    char abspath[128];
    resolve_path(arg, shell_current_path, abspath, sizeof(abspath));
    
    // Try EYNFS first
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(g_current_drive, EYNFS_SUPERBLOCK_LBA, &sb) == 0 && sb.magic == EYNFS_MAGIC) {
        eynfs_dir_entry_t entry;
        uint32_t parent_block, entry_idx;
        if (eynfs_traverse_path(g_current_drive, &sb, abspath, &entry, &parent_block, &entry_idx) == 0) {
            if (entry.type == EYNFS_TYPE_FILE) {
                // Dynamic buffer sizing based on file size
                uint32_t buffer_size;
                if (entry.size <= 16384) { // 16KB or smaller
                    buffer_size = entry.size;
                } else if (entry.size <= 65536) { // 64KB or smaller
                    buffer_size = entry.size;
                } else {
                    // For very large files, limit to 64KB
                    buffer_size = 65536; // 64KB limit for images
                }
                
                uint8_t* buffer = (uint8_t*)my_malloc(buffer_size);
                if (buffer) {
                    // Read the file (up to buffer size)
                    int bytes_to_read = (entry.size < buffer_size) ? entry.size : buffer_size;
                    int bytes_read = eynfs_read_file(g_current_drive, &sb, &entry, buffer, bytes_to_read, 0);
                    
                    if (bytes_read > 0) {
                        // Parse and display the REI image
                        rei_image_t rei_image;
                        if (rei_parse_image(buffer, bytes_read, &rei_image) == 0) {
                            printf("%cDisplaying REI image: %dx%d pixels\n", 0, 255, 0, rei_image.header.width, rei_image.header.height);
                            rei_display_image_centered(&rei_image);
                            rei_free_image(&rei_image);
                        } else {
                            printf("%cError: Invalid REI file format.\n", 255, 0, 0);
                        }
                        
                        // If we couldn't read the entire file, show a message
                        if (bytes_read < entry.size) {
                            printf("%c[File truncated - showing first %d bytes of %d total]\n", 255, 165, 0, bytes_read, entry.size);
                        }
                    } else {
                        printf("%cError: Failed to read image file.\n", 255, 0, 0);
                    }
                    
                    my_free(buffer);
                } else {
                    printf("%cError: Memory allocation failed for buffer (%d bytes).\n", 255, 0, 0, buffer_size);
                }
            } else {
                printf("%cError: Path is not a file.\n", 255, 0, 0);
            }
        } else {
            printf("%cError: File not found.\n", 255, 0, 0);
        }
    } else {
        // FAT32 fallback
        printf("%cError: No supported filesystem found.\n", 255, 0, 0);
    }
} 
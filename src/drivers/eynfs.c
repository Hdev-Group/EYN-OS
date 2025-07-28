#include "../../include/eynfs.h"
#include "../../include/types.h"
#include "../../include/string.h"
#include "../../include/vga.h"
#include "../../include/util.h"

// Forward declarations for ATA sector I/O
extern int ata_read_sector(uint8 drive, uint32 lba, uint8* buf);
extern int ata_write_sector(uint8 drive, uint32 lba, const uint8* buf);

#define EYNFS_BLOCK_SIZE 512 // For now, fixed block size

// Read the EYNFS superblock from disk
int eynfs_read_superblock(uint8 drive, uint32 lba, eynfs_superblock_t *sb) {
    uint8 buf[EYNFS_BLOCK_SIZE];
    if (ata_read_sector(drive, lba, buf) != 0) {
        return -1;
    }
    memcpy(sb, buf, sizeof(eynfs_superblock_t));
    return 0;
}

// Write the EYNFS superblock to disk
int eynfs_write_superblock(uint8 drive, uint32 lba, const eynfs_superblock_t *sb) {
    uint8 buf[EYNFS_BLOCK_SIZE] = {0};
    memcpy(buf, sb, sizeof(eynfs_superblock_t));
    if (ata_write_sector(drive, lba, buf) != 0) {
        return -1;
    }
    return 0;
}

// Read a directory table from disk (multi-block chain)
int eynfs_read_dir_table(uint8 drive, uint32 lba, eynfs_dir_entry_t *entries, size_t max_entries) {
    size_t total_entries = 0;
    uint32_t current_block = lba;
    uint8 buf[EYNFS_BLOCK_SIZE];
    while (current_block && total_entries < max_entries) {
        if (ata_read_sector(drive, current_block, buf) != 0) return -1;
        uint32_t next_block = *(uint32_t*)buf;
        size_t entry_count = (EYNFS_BLOCK_SIZE - 4) / sizeof(eynfs_dir_entry_t);
        if (max_entries - total_entries < entry_count) entry_count = max_entries - total_entries;
        memcpy(&entries[total_entries], buf + 4, entry_count * sizeof(eynfs_dir_entry_t));
        total_entries += entry_count;
        current_block = next_block;
    }
    return (int)total_entries;
}

// Write a directory table to disk (multi-block chain)
int eynfs_write_dir_table(uint8 drive, uint32 lba, const eynfs_dir_entry_t *entries, size_t num_entries) {
    uint32_t current_block = lba;
    uint32_t prev_block = 0;
    size_t written = 0;
    uint8 buf[EYNFS_BLOCK_SIZE];
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(drive, 0, &sb) != 0) return -1;
    while (written < num_entries) {
        if (!current_block) {
            int new_block = eynfs_alloc_block(drive, &sb);
            if (new_block < 0) return -1;
            if (prev_block) {
                if (ata_read_sector(drive, prev_block, buf) != 0) return -1;
                *(uint32_t*)buf = new_block;
                if (ata_write_sector(drive, prev_block, buf) != 0) return -1;
            }
            current_block = new_block;
        }
        size_t entry_count = (EYNFS_BLOCK_SIZE - 4) / sizeof(eynfs_dir_entry_t);
        size_t to_write = (num_entries - written < entry_count) ? (num_entries - written) : entry_count;
        memset(buf, 0, EYNFS_BLOCK_SIZE);
        *(uint32_t*)buf = 0;
        memcpy(buf + 4, &entries[written], to_write * sizeof(eynfs_dir_entry_t));
        if (ata_write_sector(drive, current_block, buf) != 0) return -1;
        written += to_write;
        prev_block = current_block;
        if (written < num_entries) {
            if (ata_read_sector(drive, current_block, buf) != 0) return -1;
            current_block = *(uint32_t*)buf;
        } else {
            if (ata_read_sector(drive, current_block, buf) != 0) return -1;
            uint32_t next_block = *(uint32_t*)buf;
            *(uint32_t*)buf = 0;
            if (ata_write_sector(drive, prev_block, buf) != 0) return -1;
            while (next_block) {
                uint32_t to_free = next_block;
                if (ata_read_sector(drive, to_free, buf) != 0) break;
                next_block = *(uint32_t*)buf;
                eynfs_free_block(drive, &sb, to_free);
            }
        }
    }
    return (int)written;
}

// Helper: Read the free block bitmap
static int eynfs_read_bitmap(uint8 drive, const eynfs_superblock_t *sb, uint8 *bitmap) {
    return ata_read_sector(drive, sb->free_block_map, bitmap);
}

// Helper: Write the free block bitmap
static int eynfs_write_bitmap(uint8 drive, const eynfs_superblock_t *sb, const uint8 *bitmap) {
    return ata_write_sector(drive, sb->free_block_map, bitmap);
}

// Allocate a free block, mark it as used in the bitmap, and return its block number
int eynfs_alloc_block(uint8 drive, eynfs_superblock_t *sb) {
    uint8 bitmap[EYNFS_BLOCK_SIZE];
    if (eynfs_read_bitmap(drive, sb, bitmap) != 0) return -1;
    for (uint32_t i = 0; i < EYNFS_BLOCK_SIZE * 8 && i < sb->total_blocks; ++i) {
        uint32_t byte = i / 8;
        uint8_t bit = i % 8;
        if (!(bitmap[byte] & (1 << bit))) {
            // Mark as used
            bitmap[byte] |= (1 << bit);
            if (eynfs_write_bitmap(drive, sb, bitmap) != 0) return -1;
            return i;
        }
    }
    return -1; // No free block found
}

// Free a block (mark as unused in the bitmap)
int eynfs_free_block(uint8 drive, eynfs_superblock_t *sb, uint32_t block) {
    if (block >= sb->total_blocks) return -1;
    uint8 bitmap[EYNFS_BLOCK_SIZE];
    if (eynfs_read_bitmap(drive, sb, bitmap) != 0) return -1;
    uint32_t byte = block / 8;
    uint8_t bit = block % 8;
    bitmap[byte] &= ~(1 << bit);
    if (eynfs_write_bitmap(drive, sb, bitmap) != 0) return -1;
    return 0;
}

// Find an entry by name in a directory block
// Returns 0 if found, -1 if not found
int eynfs_find_in_dir(uint8 drive, const eynfs_superblock_t *sb, uint32_t dir_block, const char *name, eynfs_dir_entry_t *out_entry, uint32_t *out_index) {
    eynfs_dir_entry_t entries[EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t)];
    int count = eynfs_read_dir_table(drive, dir_block, entries, EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t));
    if (count < 0) return -1;
    for (int i = 0; i < count; ++i) {
        if (entries[i].name[0] == '\0') continue; // Empty slot
        if (strncmp(entries[i].name, name, EYNFS_NAME_MAX) == 0) {
            if (out_entry) *out_entry = entries[i];
            if (out_index) *out_index = i;
            return 0;
        }
    }
    return -1;
}

// Traverse a path from root, return the entry for the last component
// Optionally returns parent directory block and entry index
// Returns 0 if found, -1 if not found
int eynfs_traverse_path(uint8 drive, const eynfs_superblock_t *sb, const char *path, eynfs_dir_entry_t *out_entry, uint32_t *parent_block, uint32_t *entry_index) {
    if (!path || path[0] != '/') return -1;
    // PATCH: handle root directory as a valid path
    if (strcmp(path, "/") == 0) {
        if (out_entry) {
            memset(out_entry, 0, sizeof(eynfs_dir_entry_t));
            out_entry->type = EYNFS_TYPE_DIR;
            out_entry->first_block = sb->root_dir_block;
        }
        if (parent_block) *parent_block = 0;
        if (entry_index) *entry_index = 0;
        return 0;
    }
    char temp_path[256];
    strncpy(temp_path, path, sizeof(temp_path));
    temp_path[sizeof(temp_path)-1] = '\0';
    char *token, *saveptr;
    uint32_t current_block = sb->root_dir_block;
    uint32_t last_parent = 0;
    uint32_t last_index = 0;
    eynfs_dir_entry_t entry;
    token = strtok_r(temp_path, "/", &saveptr);
    while (token) {
        if (eynfs_find_in_dir(drive, sb, current_block, token, &entry, &last_index) != 0) {
            return -1;
        }
        last_parent = current_block;
        current_block = entry.first_block;
        token = strtok_r(NULL, "/", &saveptr);
        if (token && entry.type != EYNFS_TYPE_DIR) {
            return -1; // Not a directory in the path
        }
    }
    if (out_entry) *out_entry = entry;
    if (parent_block) *parent_block = last_parent;
    if (entry_index) *entry_index = last_index;
    return 0;
}

// Create a file or directory entry in the given parent directory
int eynfs_create_entry(uint8 drive, eynfs_superblock_t *sb, uint32_t parent_block, const char *name, uint8_t type) {
    size_t max_entries = 128;
    eynfs_dir_entry_t* entries = (eynfs_dir_entry_t*)my_malloc(sizeof(eynfs_dir_entry_t) * max_entries);
    if (!entries) return -1;
    int count = eynfs_read_dir_table(drive, parent_block, entries, max_entries);
    if (count < 0) { my_free(entries); return -1; }
    int free_idx = -1;
    for (int i = 0; i < count; ++i) {
        if (entries[i].name[0] == '\0') {
            free_idx = i;
            break;
        }
    }
    if (free_idx == -1) {
        if (count >= (int)max_entries) { my_free(entries); return -1; }
        free_idx = count;
        ++count;
    }
    int new_block = eynfs_alloc_block(drive, sb);
    if (new_block < 0) { my_free(entries); return -1; }
    memset(&entries[free_idx], 0, sizeof(eynfs_dir_entry_t));
    strncpy(entries[free_idx].name, name, EYNFS_NAME_MAX-1);
    entries[free_idx].name[EYNFS_NAME_MAX-1] = '\0';
    entries[free_idx].type = type;
    entries[free_idx].first_block = new_block;
    entries[free_idx].size = 0;
    if (type == EYNFS_TYPE_DIR) {
        uint8 zero_block[EYNFS_BLOCK_SIZE] = {0};
        if (ata_write_sector(drive, new_block, zero_block) != 0) { my_free(entries); return -1; }
    }
    int res = eynfs_write_dir_table(drive, parent_block, entries, count);
    my_free(entries);
    if (res < 0) return -1;
    return 0;
}

// Delete a file or directory entry by name from the given parent directory
int eynfs_delete_entry(uint8 drive, eynfs_superblock_t *sb, uint32_t parent_block, const char *name) {
    size_t max_entries = 128;
    eynfs_dir_entry_t* entries = (eynfs_dir_entry_t*)my_malloc(sizeof(eynfs_dir_entry_t) * max_entries);
    if (!entries) return -1;
    int count = eynfs_read_dir_table(drive, parent_block, entries, max_entries);
    if (count < 0) { my_free(entries); return -1; }
    for (int i = 0; i < count; ++i) {
        if (entries[i].name[0] == '\0') continue;
        if (strncmp(entries[i].name, name, EYNFS_NAME_MAX) == 0) {
            if (eynfs_free_block(drive, sb, entries[i].first_block) != 0) { my_free(entries); return -1; }
            entries[i].name[0] = '\0';
            int res = eynfs_write_dir_table(drive, parent_block, entries, count);
            my_free(entries);
            if (res < 0) return -1;
            return 0;
        }
    }
    my_free(entries);
    return -1;
}

// Read up to bufsize bytes from a file's data block chain, starting at offset
// Returns number of bytes read, or -1 on error
int eynfs_read_file(uint8 drive, const eynfs_superblock_t *sb, const eynfs_dir_entry_t *entry, void *buf, size_t bufsize, size_t offset) {
    if (!entry || entry->type != EYNFS_TYPE_FILE) return -1;
    if (offset >= entry->size) return 0;
    uint32_t block_num = entry->first_block;
    size_t bytes_left = entry->size - offset;
    if (bufsize < bytes_left) bytes_left = bufsize;
    size_t total_read = 0;
    uint8 block[EYNFS_BLOCK_SIZE];
    size_t skip = offset;
    // Skip blocks and bytes up to offset
    while (block_num && skip >= (EYNFS_BLOCK_SIZE-4)) {
        if (ata_read_sector(drive, block_num, block) != 0) return -1;
        uint32_t next_block = *(uint32_t*)block;
        block_num = next_block;
        skip -= (EYNFS_BLOCK_SIZE-4);
    }
    // Now at the block containing the offset
    if (block_num && bytes_left > 0) {
        if (ata_read_sector(drive, block_num, block) != 0) return -1;
        uint32_t next_block = *(uint32_t*)block;
        size_t block_offset = skip;
        size_t chunk = (EYNFS_BLOCK_SIZE-4) - block_offset;
        if (chunk > bytes_left) chunk = bytes_left;
        memcpy((uint8*)buf, block+4+block_offset, chunk);
        total_read += chunk;
        bytes_left -= chunk;
        block_num = next_block;
    }
    // Read remaining blocks
    while (block_num && bytes_left > 0) {
        if (ata_read_sector(drive, block_num, block) != 0) return -1;
        uint32_t next_block = *(uint32_t*)block;
        size_t chunk = (EYNFS_BLOCK_SIZE-4) < bytes_left ? (EYNFS_BLOCK_SIZE-4) : bytes_left;
        memcpy((uint8*)buf + total_read, block+4, chunk);
        total_read += chunk;
        bytes_left -= chunk;
        block_num = next_block;
    }
    return (int)total_read;
}

// Write data to a file, creating a chain of blocks as needed
// Returns number of bytes written, or -1 on error
int eynfs_write_file(uint8 drive, eynfs_superblock_t *sb, eynfs_dir_entry_t *entry, const void *buf, size_t size, uint32_t parent_block, uint32_t entry_index) {
    if (!entry || entry->type != EYNFS_TYPE_FILE) return -1;
    
    uint32_t prev_block = 0;
    uint32_t first_block = 0;
    size_t bytes_left = size;
    size_t total_written = 0;
    const uint8* data = (const uint8*)buf;
    
    while (bytes_left > 0) {
        int new_block = eynfs_alloc_block(drive, sb);
        if (new_block < 0) return -1;
        
        if (!first_block) first_block = new_block;
        
        if (prev_block) {
            // Update previous block's next_block pointer
            uint8 tmp[EYNFS_BLOCK_SIZE];
            if (ata_read_sector(drive, prev_block, tmp) != 0) return -1;
            *(uint32_t*)tmp = new_block;
            if (ata_write_sector(drive, prev_block, tmp) != 0) return -1;
        }
        
        uint8 block[EYNFS_BLOCK_SIZE] = {0};
        size_t chunk = bytes_left < (EYNFS_BLOCK_SIZE-4) ? bytes_left : (EYNFS_BLOCK_SIZE-4);
        *(uint32_t*)block = 0; // next_block = 0 for now
        memcpy(block+4, data+total_written, chunk);
        
        if (ata_write_sector(drive, new_block, block) != 0) return -1;
        total_written += chunk;
        bytes_left -= chunk;
        prev_block = new_block;
    }
    
    // Update entry with new first block and size
    entry->first_block = first_block;
    entry->size = size;
    
    // Update directory table
    eynfs_dir_entry_t entries[EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t)];
    int count = eynfs_read_dir_table(drive, parent_block, entries, EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t));
    if (count < 0 || entry_index >= (uint32_t)count) return -1;
    entries[entry_index] = *entry;
    if (eynfs_write_dir_table(drive, parent_block, entries, count) < 0) return -1;
    
    return (int)size;
} 

// --- Unix-like File Table and Open/Close Implementation ---
#define EYNFS_MAX_OPEN_FILES 32

typedef struct {
    int used;
    uint8 drive;
    eynfs_superblock_t sb;
    eynfs_dir_entry_t entry;
    uint32_t offset;
    int mode; // 0 = read, 1 = write, 2 = append
    uint32_t parent_block;
    uint32_t entry_index;
} eynfs_file_t;

static eynfs_file_t eynfs_files[EYNFS_MAX_OPEN_FILES];

// Open a file: returns file descriptor or -1 on error
int open(const char* path, int mode) {
    for (int i = 0; i < EYNFS_MAX_OPEN_FILES; i++) {
        if (!eynfs_files[i].used) {
            eynfs_files[i].used = 1;
            eynfs_files[i].drive = 0; // TODO: support multiple drives
            eynfs_files[i].offset = 0;
            eynfs_files[i].mode = mode;
            uint8_t disk = 0;
            if (eynfs_read_superblock(disk, 2048, &eynfs_files[i].sb) != 0 || eynfs_files[i].sb.magic != EYNFS_MAGIC)
                return -1;
            uint32_t entry_idx;
            if (eynfs_find_in_dir(disk, &eynfs_files[i].sb, eynfs_files[i].sb.root_dir_block, path, &eynfs_files[i].entry, &entry_idx) != 0) {
                // If writing, create the file
                if (mode == 1) {
                    if (eynfs_create_entry(disk, &eynfs_files[i].sb, eynfs_files[i].sb.root_dir_block, path, EYNFS_TYPE_FILE) != 0)
                        return -1;
                    if (eynfs_find_in_dir(disk, &eynfs_files[i].sb, eynfs_files[i].sb.root_dir_block, path, &eynfs_files[i].entry, &entry_idx) != 0)
                        return -1;
                } else {
                    return -1;
                }
            }
            eynfs_files[i].parent_block = eynfs_files[i].sb.root_dir_block;
            eynfs_files[i].entry_index = entry_idx;
            return i;
        }
    }
    return -1;
}

// Close a file descriptor
int close(int fd) {
    if (fd < 0 || fd >= EYNFS_MAX_OPEN_FILES || !eynfs_files[fd].used)
        return -1;
    eynfs_files[fd].used = 0;
    return 0;
} 

// Read from a file descriptor
int read(int fd, void* buf, int size) {
    if (fd < 0 || fd >= EYNFS_MAX_OPEN_FILES || !eynfs_files[fd].used || eynfs_files[fd].mode != 0)
        return -1;
    eynfs_file_t* f = &eynfs_files[fd];
    if (f->offset >= f->entry.size) return 0;
    int to_read = size;
    if (f->offset + to_read > f->entry.size)
        to_read = f->entry.size - f->offset;
    int n = eynfs_read_file(f->drive, &f->sb, &f->entry, buf, to_read, f->offset);
    if (n > 0) f->offset += n;
    return n;
}

// Write to a file descriptor
int write(int fd, const void* buf, int size) {
    if (fd < 0 || fd >= EYNFS_MAX_OPEN_FILES || !eynfs_files[fd].used)
        return -1;
    eynfs_file_t* f = &eynfs_files[fd];
    if (f->mode != 1)
        return -1;
    // Always write from offset 0 for now (overwrite)
    int n = eynfs_write_file(f->drive, &f->sb, &f->entry, buf, size, f->parent_block, f->entry_index);
    if (n > 0) {
        f->offset = n;
        f->entry.size = n;
    }
    return n;
} 
#include "../../include/eynfs.h"
#include "../../include/types.h"
#include "../../include/string.h"
#include "../../include/vga.h"

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

// Read a directory table from disk (one block = multiple entries)
int eynfs_read_dir_table(uint8 drive, uint32 lba, eynfs_dir_entry_t *entries, size_t max_entries) {
    uint8 buf[EYNFS_BLOCK_SIZE];
    if (ata_read_sector(drive, lba, buf) != 0) return -1;
    size_t entry_count = EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t);
    if (max_entries < entry_count) entry_count = max_entries;
    memcpy(entries, buf, entry_count * sizeof(eynfs_dir_entry_t));
    return (int)entry_count;
}

// Write a directory table to disk (one block = multiple entries)
int eynfs_write_dir_table(uint8 drive, uint32 lba, const eynfs_dir_entry_t *entries, size_t num_entries) {
    uint8 buf[EYNFS_BLOCK_SIZE] = {0};
    size_t entry_count = EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t);
    if (num_entries > entry_count) num_entries = entry_count;
    memcpy(buf, entries, num_entries * sizeof(eynfs_dir_entry_t));
    if (ata_write_sector(drive, lba, buf) != 0) return -1;
    return (int)num_entries;
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
// type: EYNFS_TYPE_FILE or EYNFS_TYPE_DIR
// Returns 0 on success, -1 on failure
int eynfs_create_entry(uint8 drive, eynfs_superblock_t *sb, uint32_t parent_block, const char *name, uint8_t type) {
    eynfs_dir_entry_t entries[EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t)];
    int count = eynfs_read_dir_table(drive, parent_block, entries, EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t));
    if (count < 0) return -1;
    // Find a free slot
    int free_idx = -1;
    for (int i = 0; i < count; ++i) {
        if (entries[i].name[0] == '\0') {
            free_idx = i;
            break;
        }
    }
    if (free_idx == -1) return -1; // No free slot
    // Allocate a block for the new file/dir
    int new_block = eynfs_alloc_block(drive, sb);
    if (new_block < 0) return -1;
    // Fill in the new entry
    memset(&entries[free_idx], 0, sizeof(eynfs_dir_entry_t));
    strncpy(entries[free_idx].name, name, EYNFS_NAME_MAX-1);
    entries[free_idx].name[EYNFS_NAME_MAX-1] = '\0';
    entries[free_idx].type = type;
    entries[free_idx].first_block = new_block;
    entries[free_idx].size = 0;
    // If it's a directory, zero out the new directory block
    if (type == EYNFS_TYPE_DIR) {
        uint8 zero_block[EYNFS_BLOCK_SIZE] = {0};
        if (ata_write_sector(drive, new_block, zero_block) != 0) return -1;
    }
    // Write updated directory table
    if (eynfs_write_dir_table(drive, parent_block, entries, count) < 0) return -1;
    return 0;
}

// Delete a file or directory entry by name from the given parent directory
// Returns 0 on success, -1 on failure
int eynfs_delete_entry(uint8 drive, eynfs_superblock_t *sb, uint32_t parent_block, const char *name) {
    eynfs_dir_entry_t entries[EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t)];
    int count = eynfs_read_dir_table(drive, parent_block, entries, EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t));
    if (count < 0) return -1;
    for (int i = 0; i < count; ++i) {
        if (entries[i].name[0] == '\0') continue;
        if (strncmp(entries[i].name, name, EYNFS_NAME_MAX) == 0) {
            // Free the data block
            if (eynfs_free_block(drive, sb, entries[i].first_block) != 0) return -1;
            // Mark slot as empty
            entries[i].name[0] = '\0';
            // Write updated directory table
            if (eynfs_write_dir_table(drive, parent_block, entries, count) < 0) return -1;
            return 0;
        }
    }
    return -1; // Not found
}

// Read up to bufsize bytes from a file's data block
// Returns number of bytes read, or -1 on error
int eynfs_read_file(uint8 drive, const eynfs_superblock_t *sb, const eynfs_dir_entry_t *entry, void *buf, size_t bufsize) {
    if (!entry || entry->type != EYNFS_TYPE_FILE) return -1;
    uint8 block[EYNFS_BLOCK_SIZE];
    if (ata_read_sector(drive, entry->first_block, block) != 0) return -1;
    size_t to_copy = entry->size < bufsize ? entry->size : bufsize;
    memcpy(buf, block, to_copy);
    return (int)to_copy;
}

// Write up to one block of data to a file, update size in directory entry and table
// Returns number of bytes written, or -1 on error
int eynfs_write_file(uint8 drive, eynfs_superblock_t *sb, eynfs_dir_entry_t *entry, const void *buf, size_t size, uint32_t parent_block, uint32_t entry_index) {
    if (!entry || entry->type != EYNFS_TYPE_FILE) return -1;
    if (size > EYNFS_BLOCK_SIZE) size = EYNFS_BLOCK_SIZE;
    uint8 block[EYNFS_BLOCK_SIZE] = {0};
    memcpy(block, buf, size);
    if (ata_write_sector(drive, entry->first_block, block) != 0) return -1;
    // Update size in entry and directory table
    entry->size = size;
    eynfs_dir_entry_t entries[EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t)];
    int count = eynfs_read_dir_table(drive, parent_block, entries, EYNFS_BLOCK_SIZE / sizeof(eynfs_dir_entry_t));
    if (count < 0 || entry_index >= (uint32_t)count) return -1;
    entries[entry_index] = *entry;
    if (eynfs_write_dir_table(drive, parent_block, entries, count) < 0) return -1;
    return (int)size;
} 
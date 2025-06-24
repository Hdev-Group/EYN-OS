#ifndef EYNFS_H
#define EYNFS_H

#include <stdint.h>
#include <stddef.h>
#include "types.h"

// EYNFS magic number ('EYNF')
#define EYNFS_MAGIC 0x45594E46
#define EYNFS_NAME_MAX 32

// Filesystem version
#define EYNFS_VERSION 1

// Block size for EYNFS (used throughout the FS)
#define EYNFS_BLOCK_SIZE 512

// Directory entry types
typedef enum {
    EYNFS_TYPE_FILE = 1,
    EYNFS_TYPE_DIR  = 2
} eynfs_entry_type_t;

// Superblock structure (on-disk)
typedef struct {
    uint32_t magic;         // Magic number to identify EYNFS
    uint32_t version;       // Filesystem version
    uint32_t block_size;    // Block size in bytes
    uint32_t total_blocks;  // Total number of blocks
    uint32_t root_dir_block;// Block number of root directory
    uint32_t free_block_map;// Block number of free block bitmap (optional/future)
    uint32_t name_table_block; // Block number of name table
    uint32_t reserved[2];   // Reserved for future use
} eynfs_superblock_t;

// Directory entry structure (on-disk)
typedef struct {
    char name[EYNFS_NAME_MAX]; // Null-terminated name
    uint8_t type;              // File or directory
    uint8_t flags;             // Feature flags (e.g., permissions, symlink, etc.)
    uint16_t reserved;         // Reserved for future use
    uint32_t size;             // Size in bytes (for files)
    uint32_t first_block;      // First data block or subdir table block
    uint32_t extra[2];         // Reserved for future expansion
} eynfs_dir_entry_t;

// Modular FS API (function pointers for generic file operations)
typedef struct {
    int (*open)(const char *path, int mode);
    int (*read)(int fd, void *buf, size_t size);
    int (*write)(int fd, const void *buf, size_t size);
    int (*close)(int fd);
    int (*listdir)(const char *path, void *buf, size_t bufsize);
    int (*mkdir)(const char *path);
    int (*remove)(const char *path);
    // Extend with more operations as needed
} fs_ops_t;

// Function prototypes for EYNFS API
int eynfs_read_superblock(uint8 drive, uint32 lba, eynfs_superblock_t *sb);
int eynfs_write_superblock(uint8 drive, uint32 lba, const eynfs_superblock_t *sb);
int eynfs_read_dir_table(uint8 drive, uint32 lba, eynfs_dir_entry_t *entries, size_t max_entries);
int eynfs_write_dir_table(uint8 drive, uint32 lba, const eynfs_dir_entry_t *entries, size_t num_entries);
int eynfs_find_in_dir(uint8 drive, const eynfs_superblock_t *sb, uint32_t dir_block, const char *name, eynfs_dir_entry_t *out_entry, uint32_t *out_index);
int eynfs_traverse_path(uint8 drive, const eynfs_superblock_t *sb, const char *path, eynfs_dir_entry_t *out_entry, uint32_t *parent_block, uint32_t *entry_index);
int eynfs_create_entry(uint8 drive, eynfs_superblock_t *sb, uint32_t parent_block, const char *name, uint8_t type);
int eynfs_delete_entry(uint8 drive, eynfs_superblock_t *sb, uint32_t parent_block, const char *name);
int eynfs_read_file(uint8 drive, const eynfs_superblock_t *sb, const eynfs_dir_entry_t *entry, void *buf, size_t bufsize);
int eynfs_write_file(uint8 drive, eynfs_superblock_t *sb, eynfs_dir_entry_t *entry, const void *buf, size_t size, uint32_t parent_block, uint32_t entry_index);
int eynfs_alloc_block(uint8 drive, eynfs_superblock_t *sb);
int eynfs_free_block(uint8 drive, eynfs_superblock_t *sb, uint32_t block);

#endif // EYNFS_H 
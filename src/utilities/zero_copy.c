#include <zero_copy.h>
#include <util.h>
#include <vga.h>
#include <string.h>
#include <eynfs.h>
#include <stdint.h>

// Global zero-copy state
static zero_copy_file_t g_zero_copy_files[16];  // Support up to 16 zero-copy files
static uint8_t g_zero_copy_fd_count = 0;
static uint32_t g_zero_copy_stats = 0;
static uint32_t g_zero_copy_operations = 0;

// Initialize zero-copy system
void zero_copy_init(void) {
    memset(g_zero_copy_files, 0, sizeof(g_zero_copy_files));
    g_zero_copy_fd_count = 0;
    g_zero_copy_stats = 0;
    g_zero_copy_operations = 0;
}

// Open file with zero-copy operations
int zero_copy_open(const char* path, uint8_t flags) {
    if (g_zero_copy_fd_count >= 16) {
        printf("%cError: Too many zero-copy files open\n", 255, 0, 0);
        return -1;
    }
    
    // Resolve path relative to current working directory
    extern char shell_current_path[128];
    extern void resolve_path(const char* input, const char* cwd, char* out, size_t outsz);
    char abspath[128];
    resolve_path(path, shell_current_path, abspath, sizeof(abspath));

    // Use current drive
    extern uint8 g_current_drive;
    uint8_t drive = g_current_drive;
    eynfs_superblock_t sb;
    if (eynfs_read_superblock(drive, 2048, &sb) != 0) {
        printf("%cError: Cannot read filesystem\n", 255, 0, 0);
        return -1;
    }
    
    eynfs_dir_entry_t entry;
    uint32_t parent_block, entry_index;
    if (eynfs_traverse_path(drive, &sb, abspath, &entry, &parent_block, &entry_index) != 0) {
        printf("%cError: File not found: %s\n", 255, 0, 0, abspath);
        return -1;
    }
    
    if (entry.type != EYNFS_TYPE_FILE) {
        printf("%cError: Not a file: %s\n", 255, 0, 0, abspath);
        return -1;
    }
    
    // Allocate memory for the entire file (zero-copy)
    void* mapped_addr = my_malloc((int)entry.size);
    if (!mapped_addr) {
        printf("%cError: Out of memory for zero-copy mapping\n", 255, 0, 0);
        return -1;
    }
    
    // Read entire file into memory
    int bytes_read = eynfs_read_file(drive, &sb, &entry, mapped_addr, entry.size, 0);
    if (bytes_read != (int)entry.size) {
        printf("%cError: Failed to read file for zero-copy\n", 255, 0, 0);
        my_free(mapped_addr);
        return -1;
    }
    
    // Set up zero-copy file handle
    g_zero_copy_files[g_zero_copy_fd_count].mapped_address = mapped_addr;
    g_zero_copy_files[g_zero_copy_fd_count].file_size = entry.size;
    g_zero_copy_files[g_zero_copy_fd_count].current_offset = 0;
    g_zero_copy_files[g_zero_copy_fd_count].drive = drive;
    g_zero_copy_files[g_zero_copy_fd_count].flags = flags;
    g_zero_copy_files[g_zero_copy_fd_count].block_start = entry.first_block;
    g_zero_copy_files[g_zero_copy_fd_count].access_count = 0;
    g_zero_copy_files[g_zero_copy_fd_count].dirty = 0;
    
    int fd = g_zero_copy_fd_count;
    g_zero_copy_fd_count++;
    
    return fd;
}

// Close zero-copy file
int zero_copy_close(int fd) {
    if (fd < 0 || fd >= g_zero_copy_fd_count) {
        printf("%cError: Invalid file descriptor: %d\n", 255, 0, 0, fd);
        return -1;
    }
    
    zero_copy_file_t* file = &g_zero_copy_files[fd];
    
    // If file was modified, write back to disk (TODO)
    if (file->dirty && (file->flags & ZERO_COPY_WRITE_ONLY || file->flags & ZERO_COPY_READ_WRITE)) {
        // TODO: Implement write-back to filesystem
    }
    
    // Free mapped memory
    if (file->mapped_address) {
        my_free(file->mapped_address);
        file->mapped_address = NULL;
    }
    
    // Clear file handle
    memset(file, 0, sizeof(zero_copy_file_t));
    return 0;
}

// Zero-copy read operation
size_t zero_copy_read(int fd, void* buf, size_t count) {
    if (fd < 0 || fd >= g_zero_copy_fd_count) {
        printf("%cError: Invalid file descriptor: %d\n", 255, 0, 0, fd);
        return -1;
    }
    
    zero_copy_file_t* file = &g_zero_copy_files[fd];
    
    if (!(file->flags & ZERO_COPY_READ_ONLY) && !(file->flags & ZERO_COPY_READ_WRITE)) {
        printf("%cError: File not opened for reading\n", 255, 0, 0);
        return -1;
    }
    
    if (file->current_offset >= file->file_size) {
        return 0;  // End of file
    }
    
    size_t available = file->file_size - file->current_offset;
    if (count > available) count = available;
    
    uint8_t* src = (uint8_t*)file->mapped_address + file->current_offset;
    memcpy(buf, src, count);
    
    file->current_offset += count;
    file->access_count++;
    g_zero_copy_operations++;
    
    return count;
}

// Zero-copy write operation
size_t zero_copy_write(int fd, const void* buf, size_t count) {
    if (fd < 0 || fd >= g_zero_copy_fd_count) {
        printf("%cError: Invalid file descriptor: %d\n", 255, 0, 0, fd);
        return -1;
    }
    
    zero_copy_file_t* file = &g_zero_copy_files[fd];
    
    if (!(file->flags & ZERO_COPY_WRITE_ONLY) && !(file->flags & ZERO_COPY_READ_WRITE)) {
        printf("%cError: File not opened for writing\n", 255, 0, 0);
        return -1;
    }
    
    if (file->current_offset + count > file->file_size) {
        // TODO: Implement file extension safely
        printf("%cError: Cannot extend file size in zero-copy mode\n", 255, 0, 0);
        return -1;
    }
    
    uint8_t* dest = (uint8_t*)file->mapped_address + file->current_offset;
    memcpy(dest, buf, count);
    
    file->current_offset += count;
    file->access_count++;
    file->dirty = 1;  // Mark as modified
    g_zero_copy_operations++;
    
    return count;
}

// Seek in zero-copy file
int zero_copy_seek(int fd, int32_t offset, int whence) {
    if (fd < 0 || fd >= g_zero_copy_fd_count) {
        printf("%cError: Invalid file descriptor: %d\n", 255, 0, 0, fd);
        return -1;
    }
    
    zero_copy_file_t* file = &g_zero_copy_files[fd];
    uint32_t new_offset;
    
    switch (whence) {
        case 0:  new_offset = offset; break;            // SEEK_SET
        case 1:  new_offset = file->current_offset + offset; break; // SEEK_CUR
        case 2:  new_offset = file->file_size + offset; break;      // SEEK_END
        default: printf("%cError: Invalid seek whence: %d\n", 255, 0, 0, whence); return -1;
    }
    
    if (new_offset > file->file_size) {
        printf("%cError: Seek beyond end of file\n", 255, 0, 0);
        return -1;
    }
    
    file->current_offset = new_offset;
    return 0;
}

// Memory mapping for zero-copy operations
void* zero_copy_mmap(void* addr, size_t length, uint8_t flags, int fd, int32_t offset) {
    if (fd >= 0 && fd < g_zero_copy_fd_count) {
        zero_copy_file_t* file = &g_zero_copy_files[fd];
        if (offset + length > file->file_size) {
            printf("%cError: Mapping beyond file size\n", 255, 0, 0);
            return NULL;
        }
        return (uint8_t*)file->mapped_address + offset;
    } else {
        void* mapped_addr = my_malloc((int)length);
        if (!mapped_addr) {
            printf("%cError: Out of memory for anonymous mapping\n", 255, 0, 0);
            return NULL;
        }
        memset(mapped_addr, 0, length);
        return mapped_addr;
    }
}

// Unmap zero-copy memory
int zero_copy_munmap(void* addr, size_t length) {
    // TODO: Track anonymous mappings to free them explicitly.
    return 0;
}

// Synchronize zero-copy memory to disk
int zero_copy_msync(void* addr, size_t length, uint8_t flags) {
    for (int i = 0; i < g_zero_copy_fd_count; i++) {
        zero_copy_file_t* file = &g_zero_copy_files[i];
        if (file->mapped_address && 
            addr >= file->mapped_address && 
            addr < (uint8_t*)file->mapped_address + file->file_size) {
            if (file->dirty) {
                // TODO: Implement actual write-back to filesystem
                file->dirty = 0;
            }
            return 0;
        }
    }
    printf("%cError: Address not mapped\n", 255, 0, 0);
    return -1;
}

// Advanced zero-copy operations
int zero_copy_splice(int fd_in, int fd_out, size_t count) {
    if (fd_in < 0 || fd_in >= g_zero_copy_fd_count || fd_out < 0 || fd_out >= g_zero_copy_fd_count) {
        printf("%cError: Invalid file descriptors\n", 255, 0, 0);
        return -1;
    }
    zero_copy_file_t* file_in = &g_zero_copy_files[fd_in];
    zero_copy_file_t* file_out = &g_zero_copy_files[fd_out];
    size_t available_in = file_in->file_size - file_in->current_offset;
    size_t available_out = file_out->file_size - file_out->current_offset;
    if (count > available_in) count = available_in;
    if (count > available_out) count = available_out;
    uint8_t* src = (uint8_t*)file_in->mapped_address + file_in->current_offset;
    uint8_t* dest = (uint8_t*)file_out->mapped_address + file_out->current_offset;
    memcpy(dest, src, count);
    file_in->current_offset += count;
    file_out->current_offset += count;
    file_out->dirty = 1;
    g_zero_copy_operations++;
    return count;
}

int zero_copy_tee(int fd_in, int fd_out, size_t count) {
    if (fd_in < 0 || fd_in >= g_zero_copy_fd_count || fd_out < 0 || fd_out >= g_zero_copy_fd_count) {
        printf("%cError: Invalid file descriptors\n", 255, 0, 0);
        return -1;
    }
    zero_copy_file_t* file_in = &g_zero_copy_files[fd_in];
    zero_copy_file_t* file_out = &g_zero_copy_files[fd_out];
    size_t available_in = file_in->file_size - file_in->current_offset;
    size_t available_out = file_out->file_size - file_out->current_offset;
    if (count > available_in) count = available_in;
    if (count > available_out) count = available_out;
    uint8_t* src = (uint8_t*)file_in->mapped_address + file_in->current_offset;
    uint8_t* dest = (uint8_t*)file_out->mapped_address + file_out->current_offset;
    memcpy(dest, src, count);
    file_out->current_offset += count;
    file_out->dirty = 1;
    g_zero_copy_operations++;
    return count;
}

uint32_t get_zero_copy_stats(void) { return g_zero_copy_operations; }
void reset_zero_copy_stats(void) { g_zero_copy_operations = 0; for (int i=0;i<g_zero_copy_fd_count;i++){g_zero_copy_files[i].access_count=0;}}
void print_zero_copy_stats(void) {
    printf("%c=== Zero-Copy File Statistics ===\n", 255, 255, 255);
    printf("%cTotal operations: %d\n", 255, 255, 255, g_zero_copy_operations);
    printf("%cOpen files: %d\n", 255, 255, 255, g_zero_copy_fd_count);
    for (int i = 0; i < g_zero_copy_fd_count; i++) {
        zero_copy_file_t* file = &g_zero_copy_files[i];
        if (file->mapped_address) {
            printf("%cFile %d: size=%d, offset=%d, accesses=%d, dirty=%d\n", 255, 255, 255, i, file->file_size, file->current_offset, file->access_count, file->dirty);
        }
    }
} 
# EYNFS - EYN-OS Native Filesystem

EYNFS is the native filesystem designed specifically for EYN-OS. It provides a simple, efficient, and reliable storage solution optimized for the operating system's needs.

## Design Philosophy

### Goals
- **Simplicity**: Easy to understand and implement
- **Efficiency**: Minimal overhead for small files
- **Reliability**: Robust error handling and recovery
- **Compatibility**: Works with EYN-OS's freestanding environment

### Key Features
- **Block-based**: Uses 512-byte sectors for storage
- **Hierarchical**: Supports directories and subdirectories
- **Extensible**: Easy to add new features
- **Self-contained**: No external dependencies

## Filesystem Layout

### Superblock (LBA 2048)
```
Offset  Size    Field           Description
0x00    4       magic           Magic number (0x45594E46 = "EYNF")
0x04    4       version         Filesystem version (currently 1)
0x08    4       total_blocks    Total number of blocks in filesystem
0x0C    4       free_blocks     Number of free blocks
0x10    4       root_dir_block  Block number of root directory
0x14    4       block_size      Size of each block (512 bytes)
0x18    32      volume_name     Volume label (null-terminated)
0x38    480     reserved        Reserved for future use
```

### Directory Entry Structure
```
Offset  Size    Field           Description
0x00    32      name            Filename (null-terminated)
0x20    4       size            File size in bytes
0x24    4       first_block     First block number of file data
0x28    4       parent_block    Parent directory block number
0x2C    4       entry_index     Index within parent directory
0x30    1       type            Entry type (0=file, 1=directory)
0x31    1       attributes      File attributes (read-only, hidden, etc.)
0x32    2       reserved        Reserved for future use
0x34    28      padding         Padding to 64-byte alignment
```

### Block Allocation
- **Block Size**: 512 bytes (1 sector)
- **Allocation Strategy**: Simple linked list
- **Free Space Management**: Bitmap or linked list of free blocks
- **Fragmentation**: Minimal due to small block size

## Filesystem Operations

### Reading Files
1. **Find Entry**: Search directory for filename
2. **Get Block List**: Follow linked list from `first_block`
3. **Read Data**: Read blocks sequentially into buffer
4. **Return Data**: Provide file contents to caller

### Writing Files
1. **Allocate Blocks**: Find free blocks for file data
2. **Update Entry**: Modify directory entry with new size/blocks
3. **Write Data**: Write file contents to allocated blocks
4. **Update Metadata**: Update free block count and other metadata

### Creating Files
1. **Find Space**: Locate free directory entry slot
2. **Allocate Blocks**: Reserve blocks for file data
3. **Create Entry**: Initialize directory entry structure
4. **Update Parent**: Link entry into parent directory

### Deleting Files
1. **Find Entry**: Locate file in directory
2. **Free Blocks**: Mark file blocks as free
3. **Remove Entry**: Clear directory entry
4. **Update Metadata**: Update free block count

## Directory Structure

### Root Directory
- **Location**: Block specified in superblock
- **Special**: Always exists and cannot be deleted
- **Entries**: Maximum 8 entries per block (512 bytes / 64 bytes per entry)

### Subdirectories
- **Creation**: Same as regular files but with type=1
- **Traversal**: Follow parent_block links to navigate hierarchy
- **Limitations**: No hard links or symbolic links

### Path Resolution
```
/                    # Root directory
/file.txt           # File in root
/dir/               # Directory in root
/dir/file.txt       # File in subdirectory
/dir/subdir/file.txt # File in nested directory
```

## Implementation Details

### Key Functions

#### `eynfs_read_superblock(uint8 drive, uint32_t lba, eynfs_superblock_t* sb)`
Reads and validates the filesystem superblock.

#### `eynfs_find_in_dir(uint8 drive, eynfs_superblock_t* sb, uint32_t dir_block, const char* name, eynfs_dir_entry_t* entry)`
Searches a directory for a specific filename.

#### `eynfs_read_file(uint8 drive, eynfs_superblock_t* sb, eynfs_dir_entry_t* entry, void* buffer, uint32_t size, uint32_t offset)`
Reads file data from disk into memory buffer.

#### `eynfs_write_file(uint8 drive, eynfs_superblock_t* sb, eynfs_dir_entry_t* entry, const void* data, uint32_t size, uint32_t offset)`
Writes data from memory buffer to file on disk.

#### `eynfs_create_entry(uint8 drive, eynfs_superblock_t* sb, uint32_t parent_block, const char* name, uint8_t type, eynfs_dir_entry_t* new_entry)`
Creates a new file or directory entry.

#### `eynfs_traverse_path(uint8 drive, eynfs_superblock_t* sb, const char* path, eynfs_dir_entry_t* entry, uint32_t* parent_block, uint32_t* entry_index)`
Resolves a file path to its directory entry.

### Error Handling
- **Invalid Magic**: Filesystem not recognized
- **Block Not Found**: File or directory doesn't exist
- **Disk Error**: Hardware or I/O problems
- **Out of Space**: No free blocks available
- **Invalid Path**: Malformed or non-existent path

## Usage Examples

### Reading a File
```c
eynfs_superblock_t sb;
eynfs_dir_entry_t entry;

// Read superblock
if (eynfs_read_superblock(drive, EYNFS_SUPERBLOCK_LBA, &sb) != 0) {
    printf("Error: Invalid filesystem\n");
    return -1;
}

// Find file
if (eynfs_find_in_dir(drive, &sb, sb.root_dir_block, "test.txt", &entry) != 0) {
    printf("Error: File not found\n");
    return -1;
}

// Read file contents
char buffer[1024];
int bytes_read = eynfs_read_file(drive, &sb, &entry, buffer, sizeof(buffer), 0);
```

### Creating a File
```c
eynfs_dir_entry_t new_file;
const char* data = "Hello, EYN-OS!";

// Create file entry
if (eynfs_create_entry(drive, &sb, sb.root_dir_block, "hello.txt", EYNFS_TYPE_FILE, &new_file) != 0) {
    printf("Error: Could not create file\n");
    return -1;
}

// Write data to file
eynfs_write_file(drive, &sb, &new_file, data, strlen(data), 0);
```

### Directory Traversal
```c
eynfs_dir_entry_t entry;
uint32_t parent_block, entry_index;

// Navigate to nested file
if (eynfs_traverse_path(drive, &sb, "/games/snake.dat", &entry, &parent_block, &entry_index) != 0) {
    printf("Error: Path not found\n");
    return -1;
}
```

## Tools and Utilities

### Filesystem Creation
- **Format Command**: `format` command creates new EYNFS filesystem
- **Block Device**: Works with any block device (hard disk, floppy, etc.)
- **Size Calculation**: Automatically calculates filesystem size

### Maintenance
- **Integrity Check**: Validate filesystem structure
- **Defragmentation**: Reorganize files for better performance
- **Backup/Restore**: Copy filesystem to/from other storage

## Performance Characteristics

### Strengths
- **Fast Access**: Direct block addressing
- **Low Overhead**: Minimal metadata per file
- **Simple Recovery**: Easy to repair corrupted filesystems
- **Memory Efficient**: Small memory footprint

### Limitations
- **Block Size**: Fixed 512-byte blocks may waste space for small files
- **Directory Size**: Limited entries per directory block
- **No Compression**: No built-in data compression
- **No Journaling**: No transaction logging for crash recovery

## Future Enhancements

### Planned Features
- **Variable Block Sizes**: Support for different block sizes
- **File Compression**: Built-in compression for text files
- **Journaling**: Transaction logging for crash recovery
- **Extended Attributes**: Support for file metadata
- **Hard Links**: Multiple directory entries pointing to same file

### Compatibility
- **Backward Compatibility**: New features won't break existing filesystems
- **Migration Tools**: Utilities to upgrade filesystem format
- **Cross-Platform**: Tools to access EYNFS from other operating systems

---
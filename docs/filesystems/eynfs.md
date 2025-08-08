# EYNFS - EYN-OS Native Filesystem

EYNFS is the native filesystem for EYN-OS, designed for simplicity and performance.

## Overview

EYNFS is a simple, block-based filesystem that supports:
- Files and directories
- Multi-block directory chains (supports up to 128 entries per directory)
- Efficient block allocation
- Caching for performance
- Memory protection and process isolation

## Filesystem Structure

### Superblock
Located at LBA 2048, contains:
- Magic number (EYNF)
- Version information
- Block size (512 bytes)
- Total blocks
- Root directory block
- Free block bitmap location
- Name table location

### Directory Entries
Each directory entry is 52 bytes:
- 32 bytes: Null-terminated filename
- 1 byte: Entry type (file/directory)
- 1 byte: Flags
- 2 bytes: Reserved
- 4 bytes: File size
- 4 bytes: First block
- 8 bytes: Reserved for future use

### Directory Structure
Directories are stored as chains of blocks:
- Each block can hold up to 9 directory entries (508 bytes / 52 bytes per entry)
- Blocks are linked with next_block pointers
- Directory reading supports up to 128 entries across multiple blocks
- Automatic block allocation for large directories

## Key Features

### Multi-Block Directory Support
- **Previous Limitation**: Only 9 entries per directory
- **Current Implementation**: Supports up to 128 entries per directory
- **Block Chaining**: Directories automatically span multiple blocks as needed
- **Efficient Reading**: Directory reading traverses block chains seamlessly

### Performance Optimizations
- Block caching (16-entry cache)
- Directory entry caching (8-entry cache)
- Free block tracking
- Binary search for directory entries

### Security Features
- Memory bounds checking
- Process isolation
- Dangerous opcode detection
- File access validation

## Usage

### Creating Files
```bash
write filename.txt "content"
```

### Creating Directories
```bash
makedir directory_name
```

### Listing Contents
```bash
ls [depth]
```

### File Operations
```bash
read filename.txt
copy source.txt dest.txt
del filename.txt
```

## Implementation Details

### Directory Reading
The `eynfs_read_dir_table` function:
- Reads directory blocks in sequence
- Follows next_block pointers
- Supports up to 128 entries (configurable)
- Returns actual number of entries found

### Memory Management
- Dynamic allocation for large directory listings
- Proper memory cleanup
- Stack overflow prevention

### Block Allocation
- Efficient free block tracking
- Automatic block chaining for directories
- Garbage collection for deleted files

## Performance Characteristics

- **Directory Reading**: O(n) where n is number of entries
- **File Access**: O(1) for small files, O(n) for large files
- **Memory Usage**: Configurable limits (128 entries max)
- **Cache Hit Rate**: Typically >80% for repeated access

## Future Enhancements

- Extended attributes
- Symbolic links
- Compression
- Encryption
- Journaling for crash recovery
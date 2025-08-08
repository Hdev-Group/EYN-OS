# EYN-OS System Overview

EYN-OS is a freestanding x86 operating system designed with the philosophy of "reinventing the wheel" - building everything from scratch to understand and control every aspect of the system (within reason).

## Architecture Overview

### Target Platform
- **Architecture**: Intel x86 (32-bit)
- **Bootloader**: GRUB (Multiboot 1.0 compliant, ultra-minimal 6.3MB ISO)
- **Memory Model**: Flat memory model (no segmentation/paging)
- **Target Hardware**: Low-end systems (16MB RAM minimum, optimized for 128KB+ systems)
- **ISO Size**: 6.3MB (78% reduction from original 29MB)

### Core Design Principles
1. **Freestanding Environment**: No dependency on standard C libraries
2. **From-Scratch Implementation**: Custom implementations of all system components
3. **Educational Focus**: Clear, understandable code for learning purposes
4. **Modular Design**: Well-separated components for maintainability
5. **Stability & Security**: Robust error handling and graceful failure recovery
6. **Extreme Portability**: Optimized for systems with minimal RAM

## System Components

### Kernel Core
- **Boot Process**: GRUB → Kernel entry point → System initialization
- **Memory Management**: Advanced heap management with corruption detection
- **Interrupt Handling**: Intelligent exception handling with recovery mechanisms
- **Device Drivers**: VGA, keyboard, ATA disk controller

### Stability & Security Features
- **Exception Recovery**: Intelligent ISR handlers that attempt recovery instead of halting
- **Memory Protection**: Heap corruption detection, stack overflow protection
- **Command Safety**: Input validation, argument sanitization, injection prevention
- **Process Isolation**: Memory separation between kernel and user programs
- **Error Logging**: Comprehensive error tracking and reporting system

### Memory Management
- **Dynamic Heap Sizing**: Adaptive memory allocation based on available RAM
- **Corruption Detection**: Magic numbers, checksums, and block validation
- **Best-Fit Allocation**: Efficient memory allocation strategy
- **Memory Leak Detection**: Tracking of allocation and free operations
- **Conservative Limits**: Prevents excessive memory usage on low-end systems

### Filesystem Layer
- **EYNFS**: Native filesystem with custom design
- **FAT32 Support**: Compatibility with existing filesystems
- **File Operations**: Read, write, create, delete, directory traversal
- **Dynamic Buffering**: Adaptive file reading with up to 64KB buffers

### User Interface
- **TUI Framework**: Text-based user interface system
- **Shell System**: Command-line interface with streaming command architecture
- **Help System**: Interactive documentation viewer with dual-pane layout
- **File Rendering**: Support for REI images and Markdown formatting

### Development Tools
- **Assembler**: Built-in NASM-compatible assembler
- **Executable Format**: Custom EYN format for user programs
- **Program Loader**: Safe execution of user programs with process isolation

### Applications
- **Game Engine**: Framework for built-in games (Snake, Maze Runner)
- **Text Editor**: Write editor for file editing
- **Utilities**: Math functions, search, sort, random number generation
- **File Operations**: Copy, move, and advanced file management

## File Structure

```
EYN-OS/
├── src/
│   ├── boot/           # Bootloader and kernel entry
│   ├── cpu/            # CPU management (IDT, ISRs)
│   ├── drivers/        # Hardware drivers
│   ├── entry/          # Kernel main entry point
│   └── utilities/      # System utilities and applications
├── include/            # Header files
├── docs/              # Documentation (this directory)
├── testdir/           # Test files and game data
└── Makefile           # Build system
```

## Build System

### Compilation
- **Compiler**: GCC with freestanding flags
- **Linker**: Custom linker script for proper memory layout
- **Target**: ELF32 binary for GRUB compatibility

### Key Build Flags
```bash
-m32                    # 32-bit x86 target
-ffreestanding         # No standard library
-fno-stack-protector   # Disable stack protection
-Oz                    # Optimize for size
```

## Boot Process

1. **GRUB Loads**: Multiboot header recognized by GRUB
2. **Kernel Entry**: `kernel.asm` sets up initial stack and calls `kernel.c`
3. **Memory Detection**: Dynamic detection of available RAM using multiboot info
4. **System Init**: Initialize IDT, drivers, filesystem with adaptive sizing
5. **Shell Launch**: Start command-line interface with streaming commands
6. **User Interaction**: Ready for user commands

## Memory Layout

```
0x00000000 - 0x000FFFFF  # Real mode and BIOS
0x00100000 - 0x001FFFFF  # Kernel code and data
0x00200000 - 0x007FFFFF  # Available memory (adaptive heap)
0x00800000 - 0x00FFFFFF  # High memory (if available)
```

## Hardware Support

### Supported Devices
- **VGA**: Text mode display (80x25 characters)
- **PS/2 Keyboard**: Basic keyboard input
- **ATA/IDE**: Hard disk and CD-ROM access
- **Serial**: Basic serial port communication

### Memory Requirements
- **Minimum**: 16MB RAM (with optimizations)
- **Recommended**: 32MB+ RAM
- **Optimal**: 64MB+ RAM
- **Target**: Systems as low as 128KB RAM (future goal)

## Streaming Command Architecture

### Essential Commands
Always available in RAM for core functionality:
- `init`, `ls`, `exit`, `clear`, `help`
- `memory`, `portable`, `load`, `unload`, `status`

### Streaming Commands
Loaded on-demand to conserve memory:
- Filesystem: `format`, `fdisk`, `fscheck`, `copy`, `move`, `del`
- Basic: `echo`, `ver`, `calc`, `search`, `drive`, `read`, `write`
- Advanced: `random`, `history`, `sort`, `game`, `draw`
- Subcommands: Various specialized command variants

## Development Philosophy

### Why "Reinvent the Wheel"?
1. **Learning**: Understanding how everything works
2. **Control**: Full control over system behavior
3. **Simplicity**: No unnecessary complexity
4. **Customization**: Tailored to specific needs

### Code Style
- **Clear Comments**: Extensive documentation in code
- **Simple Functions**: One function, one purpose
- **Consistent Naming**: Descriptive function and variable names
- **Error Handling**: Graceful error recovery where possible
- **Professional Output**: Clean, informative user messages

## Release 12 Highlights

### Stability Improvements
- **Intelligent Exception Handling**: System attempts recovery instead of halting
- **Memory Corruption Detection**: Advanced heap validation and error reporting
- **Command Safety**: Input validation and argument sanitization
- **Process Isolation**: Memory separation for user programs

### Portability Enhancements
- **Dynamic Memory Detection**: Automatic RAM detection using multiboot info
- **Adaptive Heap Sizing**: Conservative memory allocation for low-end systems
- **Streaming Command System**: On-demand command loading to reduce memory footprint
- **Optimized File I/O**: Dynamic buffering with efficient memory usage

### User Experience
- **Professional Help System**: Interactive TUI with dual-pane layout
- **File Format Support**: REI image rendering and Markdown formatting
- **Clean Output**: Removed debug messages for professional appearance
- **Command Consistency**: All registered commands properly included in streaming system

## Future Directions

### Planned Features
- **Protected Mode**: Full 32-bit protected mode with paging
- **Multitasking**: Basic process scheduling
- **Network Support**: TCP/IP stack
- **GUI System**: Graphical user interface
- **More Games**: Additional built-in games

### Extensibility
- **Module System**: Loadable kernel modules
- **Plugin Architecture**: Extensible application framework
- **API Stability**: Stable programming interfaces
# EYN-OS System Overview

EYN-OS is a freestanding x86 operating system designed with the philosophy of "reinventing the wheel" - building everything from scratch to understand and control every aspect of the system (within reason).

## Architecture Overview

### Target Platform
- **Architecture**: Intel x86 (32-bit)
- **Bootloader**: GRUB (Multiboot 1.0 compliant, ultra-minimal 6.3MB ISO)
- **Memory Model**: Flat memory model (no segmentation/paging)
- **Target Hardware**: Low-end systems (below 16MB RAM)
- **ISO Size**: 6.3MB (78% reduction from original 29MB)

### Core Design Principles
1. **Freestanding Environment**: No dependency on standard C libraries
2. **From-Scratch Implementation**: Custom implementations of all system components
3. **Educational Focus**: Clear, understandable code for learning purposes
4. **Modular Design**: Well-separated components for maintainability

## System Components

### Kernel Core
- **Boot Process**: GRUB → Kernel entry point → System initialization
- **Memory Management**: Simple malloc/free implementation
- **Interrupt Handling**: IDT setup with custom ISRs
- **Device Drivers**: VGA, keyboard, ATA disk controller

### Filesystem Layer
- **EYNFS**: Native filesystem with custom design
- **FAT32 Support**: Compatibility with existing filesystems
- **File Operations**: Read, write, create, delete, directory traversal

### User Interface
- **TUI Framework**: Text-based user interface system
- **Shell System**: Command-line interface with command history
- **Help System**: Interactive documentation viewer

### Development Tools
- **Assembler**: Built-in NASM-compatible assembler
- **Executable Format**: Custom EYN format for user programs
- **Program Loader**: Safe execution of user programs

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
3. **System Init**: Initialize IDT, drivers, filesystem
4. **Shell Launch**: Start command-line interface
5. **User Interaction**: Ready for user commands

## Memory Layout

```
0x00000000 - 0x000FFFFF  # Real mode and BIOS
0x00100000 - 0x001FFFFF  # Kernel code and data
0x00200000 - 0x00FFFFFF  # Available memory
0x01000000 - 0xFFFFFFFF  # High memory (if available)
```

## Hardware Support

### Supported Devices
- **VGA**: Text mode display (80x25 characters)
- **PS/2 Keyboard**: Basic keyboard input
- **ATA/IDE**: Hard disk and CD-ROM access
- **Serial**: Basic serial port communication

### Memory Requirements
- **Minimum**: 1MB RAM
- **Recommended**: 4MB+ RAM
- **Optimal**: 8MB+ RAM

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

---
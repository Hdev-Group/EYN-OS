# EYN-OS - A Freestanding x86 Operating System

EYN-OS is a complete operating system built from scratch with the philosophy of "reinventing the wheel" - understanding and implementing every component of the system. It features a custom filesystem, built-in development tools, games, and a comprehensive user interface.

## Features

### Core System
- **Freestanding x86 Kernel**: Complete operating system without external dependencies
- **Custom EYNFS Filesystem**: Native filesystem designed for EYN-OS
- **FAT32 Support**: Compatibility with existing filesystems
- **Memory Management**: Custom malloc/free implementation
- **Interrupt Handling**: Complete IDT and ISR system

### User Interface
- **Text User Interface (TUI)**: Consistent interface across all applications
- **Command History**: Navigate previous commands with arrow keys
- **Interactive Help**: Comprehensive help system with TUI
- **Text Editor**: Built-in write editor for file editing

### Development Tools
- **Built-in Assembler**: NASM-compatible assembler for x86 code
- **Custom Executable Format**: EYN format for user programs
- **Program Loader**: Safe execution of user programs with static checks
- **Calculator**: Basic mathematical operations

### Games and Applications
- **Game Engine**: Framework for built-in games
- **Snake Game**: Complete implementation with TUI interface
- **Maze Runner**: Navigate through mazes while avoiding enemies
- **Game Data Files**: Games stored as `.txt` files for easy distribution

### Utilities
- **Random Number Generator**: Linear Congruential Generator
- **Sorting Algorithms**: Quicksort and bubble sort implementations
- **Search Function**: Boyer-Moore string search algorithm
- **File Operations**: Complete filesystem management

### Shell Commands
- **Filesystem**: `ls`, `cd`, `del`, `deldir`, `makedir`, `read`, `write`, `copy`, `move`
- **System**: `drive`, `format`, `fdisk`, `clear`, `help`, `history`, `exit`
- **Development**: `assemble`, `run`, `calc`
- **Games**: `game`
- **Utilities**: `random`, `sort`, `search`

## Quick Start

### Building EYN-OS
```bash
cd EYN-OS
make build
```

### Running in QEMU
```bash
make run
```

Or manually:
```bash
qemu-system-i386 -cdrom EYNOS.iso -hda tmp/boot/disk.img -hdb tmp/boot/eynfs.img -boot d
```

### Running on Real Hardware
Flash the `EYNOS.iso` to a USB drive and boot from it. Tested on Intel x86 hardware.

## Example Usage

### Basic Navigation
```
0:/! ls
0:/! cd games
0:/games! ls
0:/games! game snake
```

### Development
```
0:/! write hello.asm
0:/! assemble hello.asm hello.eyn
0:/! run hello.eyn
```

### File Management
```
0:/! makedir projects
0:/! cd projects
0:/projects! write test.txt
0:/projects! read test.txt
0:/projects! copy test.txt backup.txt
0:/projects! move backup.txt /backup/
```

## System Requirements

- **Architecture**: Intel x86 (32-bit)
- **Memory**: Minimum 1MB RAM, recommended 4MB+
- **Storage**: Any block device (hard disk, USB, etc.)
- **Display**: VGA text mode (80x25)
- **ISO Size**: 6.3MB (ultra-minimal, legacy BIOS only)

## Architecture

### Boot Process
1. **GRUB**: Multiboot 1.0 compliant bootloader
2. **Kernel**: Assembly entry point → C kernel initialization
3. **Drivers**: VGA, keyboard, ATA disk controller
4. **Filesystem**: EYNFS or FAT32 detection and mounting
5. **Shell**: Command-line interface ready for user input

### Memory Layout
- **0x00000000-0x000FFFFF**: Real mode and BIOS
- **0x00100000-0x001FFFFF**: Kernel code and data
- **0x00200000-0x00FFFFFF**: Available memory
- **0x01000000+**: High memory (if available)

### Filesystem Structure
```
/
├── games/
│   └── snake.dat
├── projects/
│   ├── hello.asm
│   └── hello.eyn
└── documents/
    └── readme.txt
```

## Development Philosophy

### Why "Reinvent the Wheel"?
- **Learning**: Understanding how everything works
- **Control**: Full control over system behavior
- **Simplicity**: No unnecessary complexity
- **Customization**: Tailored to specific needs

### Code Style
- **Clear Comments**: Extensive documentation in code
- **Simple Functions**: One function, one purpose
- **Consistent Naming**: Descriptive function and variable names
- **Error Handling**: Graceful error recovery where possible

## Documentation

Comprehensive documentation is available in the `docs/` directory:

- **[System Overview](docs/system-overview.md)**: High-level architecture
- **[EYNFS Specification](docs/filesystems/eynfs.md)**: Native filesystem details
- **[Shell System](docs/ui/shell.md)**: Command-line interface
- **[Quick Reference](docs/quick-reference.md)**: Command cheat sheet
- **[API Reference](docs/api/headers.md)**: Complete header documentation

## Contributing

EYN-OS is designed to be educational and extensible. When adding features:

1. **Follow the Style**: Match existing code style and patterns
2. **Document Everything**: Update relevant documentation
3. **Test Thoroughly**: Ensure features work correctly
4. **Keep it Simple**: Prefer simple, understandable implementations

## Future Development

### Planned Features
- **Protected Mode**: Full 32-bit protected mode with paging
- **Multitasking**: Basic process scheduling
- **Network Support**: TCP/IP stack
- **GUI System**: Graphical user interface
- **More Games**: Tetris, Pong, Space Invaders
- **File Browser**: Visual file management
- **Raycaster**: 3D graphics demo

### Extensibility
- **Module System**: Loadable kernel modules
- **Plugin Architecture**: Extensible application framework
- **API Stability**: Stable programming interfaces

## Screenshots

![EYN-OS Shell](image.png)
*The EYN-OS shell showing the help command and version information*

## License

EYN-OS is public domain software. See the UNLICENSE file for details.

---

*By Kian Gentry*

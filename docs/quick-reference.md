# EYN-OS Quick Reference

A quick reference guide for EYN-OS commands, features, and common operations.

## Getting Started

### Boot Process
1. **GRUB Menu**: Select EYN-OS from boot menu
2. **Kernel Load**: System initializes drivers and filesystem
3. **Shell Ready**: Command prompt appears: `0:/!`

### Basic Navigation
```bash
ls              # List files and directories
cd <dir>        # Change directory
cd ..           # Go to parent directory
cd /            # Go to root directory
clear           # Clear screen
```

## Filesystem Commands

| Command | Description | Example |
|---------|-------------|---------|
| `ls` | List directory contents | `ls /games` |
| `cd` | Change directory | `cd /games` |
| `del` | Delete file | `del test.txt` |
| `deldir` | Delete directory | `deldir old_dir` |
| `makedir` | Create directory | `makedir new_dir` |
| `read` | Display file contents | `read config.txt` |
| `write` | Edit file | `write document.txt` |
| `copy` | Copy file | `copy source.txt dest.txt` |
| `move` | Move file | `move file.txt /backup/` |

## System Commands

| Command | Description | Example |
|---------|-------------|---------|
| `drive` | Switch disk drive | `drive 1` |
| `format` | Format drive | `format 0` |
| `fdisk` | Disk partitioning | `fdisk` |
| `help` | Show help | `help ls` |
| `history` | Show command history | `history` |
| `exit` | Exit EYN-OS | `exit` |


## Development Tools

| Command | Description | Example |
|---------|-------------|---------|
| `assemble` | Assemble code | `assemble test.asm test.eyn` |
| `run` | Execute program | `run program.eyn` |
| `calc` | Calculator | `calc 2+2` |

## Games & Applications

| Command | Description | Example |
|---------|-------------|---------|
| `game` | Launch game | `game snake` |
| `write` | Text editor | `write file.txt` |

## Utility Commands

| Command | Description | Example |
|---------|-------------|---------|
| `random` | Generate random number | `random 100` |
| `sort` | Sort file lines | `sort data.txt` |
| `search` | Search files | `search "text"` |

## File Operations

### Creating Files
```bash
write newfile.txt    # Create and edit file
```

### Reading Files
```bash
read filename.txt    # Display file contents
```

### Copying Files
```bash
copy source.txt dest.txt  # Copy file
read source.txt > dest.txt  # Alternative copy method
```

### Moving Files
```bash
move file.txt /backup/     # Move file to directory
move old.txt new.txt       # Rename file
```

### Deleting Files
```bash
del filename.txt     # Delete file
deldir directory     # Delete directory
```

## Common Tasks

### Setting Up Development
```bash
makedir projects     # Create projects directory
cd projects          # Navigate to projects
write hello.asm      # Create assembly file
assemble hello.asm hello.eyn  # Assemble code
run hello.eyn        # Execute program
```

### File Management
```bash
ls                   # See what's in current directory
makedir backup       # Create backup directory
copy important.txt backup/important.txt  # Backup file
```

### System Maintenance
```bash
sysinfo              # Check system status
drive 0              # Switch to main drive
ls                   # Check filesystem
format 1             # Format secondary drive
```



## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| **Arrow Keys** | Navigate command history |
| **Backspace** | Delete character |
| **Enter** | Execute command |
| **Escape** | Clear current input |
| **Ctrl+C** | Interrupt operation |

## Game Controls

### Snake Game
- **WASD**: Move snake
- **Q**: Quit game
- **P**: Pause/unpause

### Write Editor
- **Arrow Keys**: Move cursor
- **Ctrl+O**: Save file
- **Ctrl+X**: Exit editor

## Configuration

### Drive Selection
- **Numeric drives** (0, 1, 2...): Permanent storage
- **RAM drive**: Special memory-based storage

### Path Resolution
- **Absolute paths**: Start with `/` (e.g., `/games/snake.dat`)
- **Relative paths**: No leading `/` (e.g., `games/snake.dat`)

## Troubleshooting

### Common Issues

**"No supported filesystem found"**
- Drive not formatted with EYNFS
- Use `format <drive>` to create filesystem

**"File not found"**
- Check path with `ls`
- Verify filename spelling
- Use `cd` to navigate to correct directory

**"Unknown command"**
- Use `help` to see available commands
- Check command spelling
- Use `history` to see previous commands

**"Memory allocation failed"**
- System running low on memory
- Use `sysinfo` to check memory status
- Restart system if necessary

### Getting Help
```bash
help                # Show all commands
help <command>      # Show help for specific command
```

## System Information

### Memory Layout
- **Kernel**: 0x00100000 - 0x001FFFFF
- **Available**: 0x00200000 - 0x00FFFFFF
- **High Memory**: 0x01000000+ (if available)

### Filesystem
- **EYNFS**: Native filesystem
- **FAT32**: Compatible filesystem
- **Block Size**: 512 bytes
- **Superblock**: LBA 2048

### Hardware Support
- **VGA**: 80x25 text mode
- **PS/2 Keyboard**: Full keyboard support
- **ATA/IDE**: Hard disk access
- **Serial**: Basic serial communication

## Advanced Features

### Command History
- **Navigation**: Arrow keys to browse history
- **Search**: Quick access to previous commands
- **Persistence**: History saved between sessions

### TUI System
- **Consistent Interface**: All TUI apps use same framework
- **Color Support**: Multiple colors for different elements
- **Keyboard Handling**: Unified input processing

### Game Engine
- **Modular Design**: Easy to add new games
- **Data Files**: Games stored as `.txt` files
- **TUI Integration**: Games use TUI framework



---
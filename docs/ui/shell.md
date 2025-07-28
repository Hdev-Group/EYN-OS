# EYN-OS Shell System

The EYN-OS shell provides a command-line interface for interacting with the operating system. It features command history, tab completion, and a rich set of built-in commands.

## Shell Architecture

### Core Components
- **Command Parser**: Processes user input and dispatches commands
- **History System**: Stores and retrieves previous commands
- **Input Handler**: Manages keyboard input and special keys
- **Command Registry**: Maintains list of available commands

### Design Philosophy
- **Simple and Fast**: Minimal overhead for quick response
- **User-Friendly**: Clear error messages and helpful feedback
- **Extensible**: Easy to add new commands
- **Consistent**: Uniform command interface

## ⌨User Interface

### Prompt Format
```
<drive>:/<path>! 
```
- **Drive**: Current disk drive (0, 1, 2, etc.)
- **Path**: Current working directory
- **!**: Command prompt indicator

### Examples
```
0:/!           # Root directory on drive 0
1:/games!      # Games directory on drive 1
RAM:/!         # RAM disk (special drive)
```

### Special Keys
- **Arrow Keys**: Navigate command history
- **Backspace**: Delete character
- **Enter**: Execute command
- **Escape**: Clear current input
- **Ctrl+C**: Interrupt current operation

## 📋 Built-in Commands

### Filesystem Commands

#### `ls [path]`
List directory contents.
```bash
ls              # List current directory
ls /games       # List games directory
ls -l           # Long format (if supported)
```

#### `cd <directory>`
Change current working directory.
```bash
cd /games       # Navigate to games directory
cd ..           # Go to parent directory
cd /            # Go to root directory
```

#### `del <filename>`
Delete a file.
```bash
del test.txt    # Delete test.txt
del *.tmp       # Delete all .tmp files (if supported)
```

#### `deldir <directory>`
Delete a directory.
```bash
deldir old_dir  # Delete old_dir directory
```

#### `makedir <directory>`
Create a new directory.
```bash
makedir new_dir # Create new directory
```

### File Operations

#### `read <filename>`
Display file contents (equivalent to `cat`).
```bash
read test.txt   # Display test.txt contents
```

#### `write <filename>`
Open text editor for file.
```bash
write test.txt  # Edit test.txt in write editor
```

#### `run <filename>`
Execute an EYN executable file.
```bash
run program.eyn # Execute program.eyn
```

### Development Tools

#### `assemble <input> <output>`
Assemble NASM-compatible assembly code.
```bash
assemble test.asm test.eyn  # Assemble test.asm to test.eyn
```

#### `calc <expression>`
Simple calculator.
```bash
calc 2+2        # Calculate 2+2
calc 10*5       # Calculate 10*5
```

### System Commands

#### `drive <number>`
Switch to different disk drive.
```bash
drive 0         # Switch to drive 0
drive 1         # Switch to drive 1
drive ram       # Switch to RAM disk
```

#### `clear`
Clear the screen.
```bash
clear           # Clear terminal screen
```

#### `help [command]`
Show help information.
```bash
help            # Show all commands
help ls         # Show help for ls command
```

#### `history`
Show command history.
```bash
history         # Display recent commands
```

#### `exit`
Exit the shell (returns to kernel).
```bash
exit            # Exit EYN-OS
```

### Utility Commands

#### `random [max]`
Generate random number.
```bash
random          # Random number 0-32767
random 100      # Random number 0-99
```

#### `sort <filename>`
Sort lines in a file.
```bash
sort data.txt   # Sort data.txt alphabetically
```

#### `search <pattern> [options]`
Search for text in files.
```bash
search "hello"              # Search for "hello" in all files
search "test" -f            # Search filenames only
search "pattern" -c         # Search file contents only
```

#### `game <filename>`
Launch a game.
```bash
game snake      # Launch snake game
game snake.dat  # Launch snake game (with extension)
```

### Filesystem Management

#### `format <drive>`
Format a drive with EYNFS.
```bash
format 0        # Format drive 0
```

#### `fdisk`
Disk partitioning tool.
```bash
fdisk           # Interactive disk partitioning
```

## Command System

### Command Registration
Commands are registered using the `REGISTER_SHELL_COMMAND` macro:
```c
REGISTER_SHELL_COMMAND("ls", "List directory contents", ls_cmd);
REGISTER_SHELL_COMMAND("cd", "Change directory", cd_cmd);
```

### Command Function Signature
```c
void command_name(string args);
```
- **args**: Command arguments as single string
- **Return**: void (use printf for output)

### Error Handling
Commands should provide clear error messages:
```c
void example_cmd(string args) {
    if (!args || strlen(args) == 0) {
        printf("Error: Missing argument\n");
        printf("Usage: example <argument>\n");
        return;
    }
    // Command logic here
}
```

## Command History

### Features
- **Persistent Storage**: Commands saved between sessions
- **Navigation**: Arrow keys to browse history
- **Search**: Quick access to previous commands
- **Size Limit**: Configurable history size (default: 100 commands)

### Implementation
```c
// Add command to history
add_to_history("ls /games");

// Retrieve from history
const char* cmd = get_history_entry(index);

// Clear history
clear_history();
```

## TUI Integration

### Help Command
The `help` command uses the TUI system to display:
- **Command List**: All available commands
- **Descriptions**: Brief explanation of each command
- **Usage Examples**: How to use commands
- **Navigation**: Arrow keys to browse commands

### Game Integration
Games launched via the `game` command:
- **Full Screen**: Take over entire terminal
- **TUI Framework**: Use TUI for game interface
- **Return to Shell**: Clean return after game ends

## Input Processing

### Key Handling
```c
int key = tui_read_key();
switch (key) {
    case 0x1001: // Up arrow
        // Navigate history up
        break;
    case 0x1002: // Down arrow
        // Navigate history down
        break;
    case '\n':   // Enter
        // Execute command
        break;
}
```

### Command Parsing
```c
// Split command and arguments
char* cmd = strtok(input, " ");
char* args = strtok(NULL, "");

// Find and execute command
for (int i = 0; i < command_count; i++) {
    if (strcmp(cmd, commands[i].name) == 0) {
        commands[i].function(args);
        return;
    }
}
printf("Error: Unknown command '%s'\n", cmd);
```

## Performance Features

### Optimizations
- **Command Caching**: Frequently used commands cached
- **Input Buffering**: Efficient keyboard input handling
- **Memory Management**: Minimal memory usage
- **Fast Dispatch**: Quick command lookup and execution

### Memory Usage
- **Shell Process**: ~2KB base memory
- **Command History**: ~1KB per 10 commands
- **Input Buffer**: 256 bytes
- **Command Registry**: ~1KB for all commands

## Future Enhancements

### Planned Features
- **Tab Completion**: Auto-complete filenames and commands
- **Aliases**: Command shortcuts and aliases
- **Scripting**: Basic shell scripting support
- **Job Control**: Background process management
- **Environment Variables**: System and user variables

### Extensibility
- **Plugin System**: Loadable command modules
- **Custom Prompts**: User-configurable prompt format
- **Key Bindings**: Customizable keyboard shortcuts
- **Themes**: Visual customization options

## Development

### Adding New Commands
1. **Implement Function**: Create command function
2. **Register Command**: Use `REGISTER_SHELL_COMMAND` macro
3. **Add to Help**: Update help system documentation
4. **Test**: Verify command works correctly

### Example New Command
```c
void my_cmd(string args) {
    if (!args) {
        printf("Usage: my_cmd <argument>\n");
        return;
    }
    printf("My command executed with: %s\n", args);
}

// In shell initialization
REGISTER_SHELL_COMMAND("my_cmd", "My custom command", my_cmd);
```

---

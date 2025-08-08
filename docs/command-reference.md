# EYN-OS Command Reference

This document is auto-generated from the source code. Last updated: 2025-08-08 09:29:29

**Total Commands:** 38

## Table of Contents

- [Essential Commands](#essential-commands)
- [Streaming Commands](#streaming-commands)
- [Filesystem Commands](#filesystem-commands)
- [System Commands](#system-commands)
- [Utility Commands](#utility-commands)
- [Development Commands](#development-commands)

## Essential Commands

### clear

**Handler:** `clear_cmd`

**Type:** CMD_ESSENTIAL

**File:** `shell_commands.c`

**Description:**
Clears the screen and resets the shell display.
Usage: clear

**Example:**
```bash
clear
```

---

### exit

**Handler:** `handler_exit`

**Type:** CMD_ESSENTIAL

**File:** `shell_commands.c`

**Description:**
Exits the kernel and shuts down the system.
Usage: exit

**Example:**
```bash
exit
```

---

### help

**Handler:** `help_cmd`

**Type:** CMD_ESSENTIAL

**File:** `shell_commands.c`

**Description:**
Display this message and show all available commands with descriptions and examples.
Usage: help

**Example:**
```bash
help
```

---

### init

**Handler:** `init_cmd`

**Type:** CMD_ESSENTIAL

**File:** `shell_commands.c`

**Description:**
Initialize full system services (ATA drives, etc.).
Usage: init

**Example:**
```bash
init
```

---

### memory

**Handler:** `memory_cmd`

**Type:** CMD_ESSENTIAL

**File:** `shell_commands.c`

**Description:**
Memory management and testing.
Usage: memory stats | test | stress

**Example:**
```bash
memory stats
```

---

### portable

**Handler:** `portable_cmd`

**Type:** CMD_ESSENTIAL

**File:** `shell_commands.c`

**Description:**
Display portability optimizations and memory usage.
Usage: portable [stats|optimize]

**Example:**
```bash
portable
```

---

## Streaming Commands

### catram

**Handler:** `catram_cmd`

**Type:** CMD_STREAMING

**File:** `shell_commands.c`

**Description:**
Display contents of a file from RAM disk (FAT32).
Usage: catram <filename>

**Example:**
```bash
catram test.txt
```

---

### error

**Handler:** `error_cmd`

**Type:** CMD_STREAMING

**File:** `shell_commands.c`

**Description:**
Display system error statistics and status.
Usage: error [clear|details]

**Example:**
```bash
error
```

---

### game

**Handler:** `game_cmd`

**Type:** CMD_STREAMING

**File:** `shell_commands.c`

**Description:**
Load and run a game from a .dat file.
Usage: game <filename>
Example: game snake

**Example:**
```bash
game snake
```

---

### lsram

**Handler:** `lsram_cmd`

**Type:** CMD_STREAMING

**File:** `shell_commands.c`

**Description:**
List files in the RAM disk (FAT32) with directory tree.
Usage: lsram

**Example:**
```bash
lsram
```

---

### size

**Handler:** `size`

**Type:** CMD_STREAMING

**File:** `fs_commands.c`

**Description:**
Show the size of a file in bytes.
Usage: size <filename>

**Example:**
```bash
size myfile.txt
```

---

### validate

**Handler:** `validate_cmd`

**Type:** CMD_STREAMING

**File:** `shell_commands.c`

**Description:**
Display input validation statistics and test validation.
Usage: validate [test|stats]

**Example:**
```bash
validate
```

---

## Filesystem Commands

### cd

**Handler:** `cd`

**Type:** CMD_STREAMING

**File:** `fs_commands.c`

**Description:**
Change the current directory.
Usage: cd <directory>

**Example:**
```bash
cd myfolder
```

---

### copy

**Handler:** `copy_cmd`

**Type:** CMD_STREAMING

**File:** `fs_commands.c`

**Description:**
Copy a file from source to destination.
Usage: copy <source> <destination>

**Example:**
```bash
copy file1.txt file2.txt
```

---

### del

**Handler:** `del`

**Type:** CMD_STREAMING

**File:** `fs_commands.c`

**Description:**
Delete a file from the filesystem.
Usage: del <filename>

**Example:**
```bash
del myfile.txt
```

---

### deldir

**Handler:** `deldir`

**Type:** CMD_STREAMING

**File:** `fs_commands.c`

**Description:**
Delete an empty directory.
Usage: deldir <directory>

**Example:**
```bash
deldir myfolder
```

---

### fdisk

**Handler:** `fdisk_cmd_handler`

**Type:** CMD_STREAMING

**File:** `fdisk_commands.c`

**Description:**
List partition table or create partitions.
Usage: fdisk [create <start_lba> <size> <type>]

**Example:**
```bash
fdisk create 2048 1024000 0x0C
```

---

### format

**Handler:** `format_cmd_handler`

**Type:** CMD_STREAMING

**File:** `format_command.c`

**Description:**
Format partition n (0-3) as FAT32 or EYNFS.
FAT32: widely supported, max 4GB files.
EYNFS: native, supports long filenames, fast directory access.
Usage: format <partition_num> <filesystem_type>

**Example:**
```bash
format 1 fat32
```

---

### fscheck

**Handler:** `fscheck`

**Type:** CMD_STREAMING

**File:** `fs_commands.c`

**Description:**
Check filesystem integrity.
Usage: fscheck

**Example:**
```bash
fscheck
```

---

### ls

**Handler:** `ls_cmd`

**Type:** CMD_STREAMING

**File:** `fs_commands.c`

**Description:**
List files in the root directory of the selected drive.
Usage: ls

**Example:**
```bash
ls
```

---

### makedir

**Handler:** `makedir`

**Type:** CMD_STREAMING

**File:** `fs_commands.c`

**Description:**
Create a new directory.
Usage: makedir <directory>

**Example:**
```bash
makedir myfolder
```

---

### move

**Handler:** `move_cmd`

**Type:** CMD_STREAMING

**File:** `fs_commands.c`

**Description:**
Move a file from source to destination.
Usage: move <source> <destination>

**Example:**
```bash
move file1.txt /backup/file1.txt
```

---

### read

**Handler:** `read_cmd`

**Type:** CMD_STREAMING

**File:** `fs_commands.c`

**Description:**
Smart file display - detects file type and displays appropriately.
Usage: read <filename>

**Example:**
```bash
read myfile.txt
```

---

### write

**Handler:** `write_cmd`

**Type:** CMD_STREAMING

**File:** `fs_commands.c`

**Description:**
Open nano-like text editor for a file.
Usage: write <filename>

**Example:**
```bash
write myfile.txt
```

---

## System Commands

### drive

**Handler:** `drive_cmd`

**Type:** CMD_STREAMING

**File:** `shell_commands.c`

**Description:**
Change between different drives (from lsata).
Usage: drive <n>

**Example:**
```bash
drive 0
```

---

### lsata

**Handler:** `lsata_cmd`

**Type:** CMD_STREAMING

**File:** `shell_commands.c`

**Description:**
List detected ATA drives and their details.
Usage: lsata

**Example:**
```bash
lsata
```

---

## Utility Commands

### calc

**Handler:** `calc_cmd`

**Type:** CMD_STREAMING

**File:** `shell_commands.c`

**Description:**
32-bit fixed-point calculator. Supports +, -, *, /.
Usage: calc <expression>

**Example:**
```bash
calc 2.5+3.7
```

---

### draw

**Handler:** `draw_cmd_handler`

**Type:** CMD_STREAMING

**File:** `shell_commands.c`

**Description:**
Draw a rectangle.
Usage: draw <x> <y> <width> <height> <r> <g> <b>.
Example: draw 10 20 100 50 255 0 0 draws a red rectangle.

**Example:**
```bash
draw 10 20 100 50 255 0 0
```

---

### echo

**Handler:** `echo_cmd`

**Type:** CMD_STREAMING

**File:** `shell_commands.c`

**Description:**
Reprints a given text to the screen.
Usage: echo <text>

**Example:**
```bash
echo Hello, world!
```

---

### history

**Handler:** `history_cmd`

**Type:** CMD_STREAMING

**File:** `history.c`

**Description:**
Show or clear command history.
Usage: history [clear]
Example: history | history clear

**Example:**
```bash
history
```

---

### log

**Handler:** `log_cmd`

**Type:** CMD_STREAMING

**File:** `shell_commands.c`

**Description:**
Enable or disable shell logging.
Usage: log on|off

**Example:**
```bash
log on
```

---

### random

**Handler:** `random_cmd`

**Type:** CMD_STREAMING

**File:** `shell_commands.c`

**Description:**
Generate random numbers.
Usage: random [count] | random [min] [max]
Example: random 5 | random 10 20

**Example:**
```bash
random 5
```

---

### search

**Handler:** `search_cmd`

**Type:** CMD_STREAMING

**File:** `shell_commands.c`

**Description:**
Search for text in filenames and file contents using Boyer-Moore algorithm.
Usage: search <pattern> [-f|-c|-a]
Example: search hello -a

**Example:**
```bash
search hello -a
```

---

### sort

**Handler:** `sort_cmd`

**Type:** CMD_STREAMING

**File:** `shell_commands.c`

**Description:**
Sort strings alphabetically.
Usage: sort <string1> <string2> <string3> ...
Example: sort zebra apple banana

**Example:**
```bash
sort zebra apple banana
```

---

### spam

**Handler:** `spam_cmd`

**Type:** CMD_STREAMING

**File:** `shell_commands.c`

**Description:**
Spam 'EYN-OS' to the shell 100 times for fun.
Usage: spam

**Example:**
```bash
spam
```

---

### ver

**Handler:** `ver_cmd`

**Type:** CMD_STREAMING

**File:** `shell_commands.c`

**Description:**
Shows the current system version and release information.
Usage: ver

**Example:**
```bash
ver
```

---

## Development Commands

### assemble

**Handler:** `handler_assemble`

**Type:** CMD_STREAMING

**File:** `assemble.c`

**Description:**
Converts assembly code into machine code.
Supports NASM syntax.
Usage: assemble <input file> <output file>

**Example:**
```bash
assemble example.asm example.eyn
```

---

### run

**Handler:** `run_cmd`

**Type:** CMD_STREAMING

**File:** `run_command.c`

**Description:**
Run a .eyn executable with process isolation.
Usage: run <program.eyn>

**Example:**
```bash
run test.eyn
```

---

## Command Statistics

| Category | Count |
|----------|-------|
| Essential Commands | 6 |
| Streaming Commands | 6 |
| Filesystem Commands | 12 |
| System Commands | 2 |
| Utility Commands | 10 |
| Development Commands | 2 |


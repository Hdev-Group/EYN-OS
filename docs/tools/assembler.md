# EYN-OS Assembler

The EYN-OS assembler is a built-in NASM-compatible assembler that allows users to write and compile assembly code directly within the operating system.

## Overview

The assembler converts NASM-compatible assembly code into EYN executable format (.eyn files) that can be run by the EYN-OS program loader. It provides a complete development environment without requiring external tools.

## Architecture

### Components

#### Lexer
- **Purpose**: Tokenizes assembly source code
- **Input**: Raw assembly text
- **Output**: Stream of tokens (instructions, operands, labels, etc.)
- **Features**: Handles comments, whitespace, and basic syntax validation

#### Parser
- **Purpose**: Converts tokens into abstract syntax tree (AST)
- **Input**: Token stream from lexer
- **Output**: Structured representation of assembly code
- **Features**: Validates instruction syntax and operand types

#### Symbol Table
- **Purpose**: Manages labels and symbols
- **Features**: 
  - Label resolution and address calculation
  - Forward reference handling
  - Symbol scope management
- **Implementation**: Hash table for efficient symbol lookup

#### Code Generator
- **Purpose**: Converts AST into machine code
- **Input**: Parsed assembly code
- **Output**: EYN executable format
- **Features**: Instruction encoding, relocation handling

## Supported Instructions

### Data Movement
- `mov` - Move data between registers and memory

### Arithmetic
- `add` - Addition
- `sub` - Subtraction

### Control Flow
- `jmp` - Unconditional jump
- `call` - Call subroutine
- `ret` - Return from subroutine

### System Instructions
- `int` - Software interrupt

## Addressing Modes

### Register Addressing
```assembly
mov eax, ebx        ; Register to register
```

### Immediate Addressing
```assembly
mov eax, 42         ; Immediate value
mov ebx, 0x12345678 ; Hexadecimal immediate
```

### Memory Addressing
```assembly
mov eax, [label]    ; Direct addressing (labels only)
```

## Labels and Symbols

### Label Definition
```assembly
start:
    mov eax, 1
    ret

data_section:
    db "Hello, World!", 0
```

### Forward References
```assembly
    jmp target      ; Forward reference
    ; ... code ...
target:
    ret
```

## Directives

### Section Directives
```assembly
section .text       ; Code section
section .data       ; Data section
section .bss        ; Uninitialized data
```

### Data Directives
```assembly
db 1, 2, 3         ; Define bytes
dw 1000, 2000      ; Define words
dd 0x12345678      ; Define double words
```

### String Directives
```assembly
db "Hello", 0      ; Null-terminated string
db 'A', 'B', 'C'   ; Character array
```

## Usage

### Command Syntax
```bash
assemble <input_file> <output_file>
```

### Example Assembly Program
```assembly
; Simple "Hello, World!" program
section .text
global _start

_start:
    mov eax, 4          ; syscall number for write
    mov ebx, 1          ; file descriptor (stdout)
    mov ecx, message    ; message to write
    mov edx, 13         ; message length
    int 0x80            ; make syscall
    
    mov eax, 1          ; syscall number for exit
    mov ebx, 0          ; exit code
    int 0x80            ; make syscall

section .data
message:
    db "Hello, World!", 10  ; 10 is newline
```

### Compilation Process
1. **Source Input**: Read assembly file from EYNFS
2. **Lexical Analysis**: Convert text to tokens
3. **Parsing**: Build abstract syntax tree
4. **Symbol Resolution**: Resolve all labels and references
5. **Code Generation**: Generate machine code
6. **Output**: Write EYN executable file

## Error Handling

### Common Errors
- **Syntax Error**: Invalid instruction or operand
- **Undefined Label**: Reference to non-existent label
- **Invalid Addressing**: Unsupported addressing mode
- **File Error**: Cannot read input or write output

### Error Reporting
```bash
Error: Syntax error at line 5: invalid operand
Error: Undefined label 'target' at line 10
Error: Cannot open input file 'test.asm'
```

## Integration with EYN-OS

### EYN Executable Format
The assembler generates files in the EYN executable format:
- **Header**: Magic number and metadata
- **Code Section**: Machine code instructions
- **Data Section**: Initialized data
- **Symbol Table**: Debug information (optional)

### Program Loading
Assembled programs can be executed using the `run` command:
```bash
assemble program.asm program.eyn
run program.eyn
```

## Limitations

### Current Limitations
- **Instruction Set**: Limited to 7 basic instructions (mov, add, sub, jmp, call, ret, int)
- **Registers**: Only 8 registers supported (eax, ebx, ecx, edx, esi, edi, esp, ebp)
- **Directives**: Only 4 directives supported (db, dw, dd, global, section)
- **Optimization**: No code optimization performed
- **Debugging**: Limited debug information
- **Libraries**: No standard library support
- **Addressing**: Limited addressing modes (register, immediate, label-based memory)

### Future Enhancements
- **Extended Instructions**: Support for more x86 instructions
- **Optimization**: Basic code optimization
- **Debug Symbols**: Enhanced debugging support
- **Macros**: Macro preprocessing support
- **Libraries**: Standard library integration

## Development

### Adding New Instructions
1. **Update Lexer**: Add instruction token recognition
2. **Update Parser**: Add instruction parsing logic
3. **Update Code Generator**: Add instruction encoding
4. **Test**: Verify instruction works correctly

### Code Structure
```
src/utilities/shell/assemble.c  # Main assembler implementation
include/assemble.h              # Type definitions and prototypes
```

### Testing
- **Unit Tests**: Test individual components
- **Integration Tests**: Test complete compilation
- **Regression Tests**: Ensure existing code still works

---

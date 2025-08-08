# EYN-OS Assembler

The EYN-OS assembler is a custom NASM-compatible assembler that converts assembly code into the EYN executable format. It supports the full i386 instruction set and provides comprehensive error reporting and memory management.

## Architecture

The assembler consists of four main components:

1. **Lexer** - Tokenizes the input source code
2. **Parser** - Builds an Abstract Syntax Tree (AST) from tokens
3. **Symbol Table** - Manages labels and their addresses
4. **Code Generator** - Emits machine code from the AST

## Supported Features

### Instruction Set
The assembler supports the full i386 instruction set with over 200 instructions across all categories:

- **Data Movement**: mov, push, pop, lea, xchg, movsx, movzx
- **Arithmetic**: add, sub, inc, dec, mul, div, imul, idiv, neg, cmp
- **Logical**: and, or, xor, not, test
- **Shifts**: shl, shr, sal, sar, rol, ror, rcl, rcr
- **Control Flow**: jmp, call, ret, jz, jnz, jg, jl, je, jne, ja, jb
- **String**: movs, lods, stos, cmps, scas
- **System**: int, cli, sti, hlt, in, out

### Registers
- **32-bit**: eax, ebx, ecx, edx, esi, edi, esp, ebp
- **16-bit**: ax, bx, cx, dx, si, di, sp, bp
- **8-bit**: al, cl, dl, bl, ah, ch, dh, bh
- **Segment**: es, cs, ss, ds, fs, gs

### Directives
- `section .text` - Code section
- `section .data` - Data section
- `global <symbol>` - Export symbol
- `db`, `dw`, `dd` - Data definitions

### Addressing Modes
- Register: `mov eax, ebx`
- Immediate: `mov eax, 42`
- Label-based memory: `mov eax, [label]`

## Usage

### Basic Assembly
```bash
assemble input.asm output.eyn
```

### Running Programs
```bash
run output.eyn
```

## Error Reporting

The assembler provides comprehensive error reporting with colored output:

- **Red Errors**: Syntax errors, unknown instructions, memory allocation failures
- **Pink Warnings**: Unused labels, data definitions, potential issues

Error messages include:
- File name and line number
- Context information
- Specific error descriptions

## Memory Management

The assembler uses intelligent memory management:

- **Dynamic Sizing**: Estimates required buffer sizes based on AST content
- **Size Caps**: Maximum 16KB for code and data sections
- **Overflow Detection**: Automatic buffer overflow detection and reporting
- **Memory Safety**: Proper cleanup on allocation failures

## Safety Features

The EYN-OS executable loader includes security restrictions that block potentially dangerous instructions:

- `hlt` (halt) - 0xF4
- `cli` (clear interrupt flag) - 0xFA  
- `sti` (set interrupt flag) - 0xFB
- `int` (software interrupt) - 0xCD
- `in` (input from port) - 0xE4, 0xEC, 0xE5, 0xED
- `out` (output to port) - 0xE6, 0xEE, 0xE7, 0xEF

## Example Programs

### Hello World (testdir/hello_world.asm)
Demonstrates comprehensive assembly features:
- Register operations and arithmetic
- Logical operations (and, or, xor)
- Shift operations (shl, shr)
- Control flow with loops and conditionals
- Data section usage

### Simple Test (testdir/test_hello.asm)
Minimal program for testing basic assembly functionality.

## Current Limitations

- **Optimization**: No code optimization performed
- **Debugging**: Limited debug information
- **Libraries**: No standard library support
- **Advanced Features**: No MMX/SSE support, limited memory addressing modes
- **Safety Restrictions**: Some instructions are blocked by the EYN-OS executable loader for security

## Future Enhancements

- **Advanced Directives**: Include files, macros, more data directives
- **Optimization**: Basic code optimization
- **Debug Support**: Debug information generation
- **Extended Addressing**: More complex memory addressing modes
- **Library Support**: Standard library integration
- **Output Formats**: Listing files, symbol table dumps
- **Integration**: Shell autocomplete, return code propagation

## Development Tips

1. Start with simple programs to test the assembler
2. Use the `fscheck` command to verify filesystem integrity
3. Check for "unsupported instruction" errors and report them
4. Use `Ctrl+C` to interrupt running programs
5. Monitor memory usage with large programs
6. Use colored error messages to quickly identify issues

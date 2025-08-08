; Medium-sized test program to check buffer limits
section .text
global _start

_start:
    ; Basic arithmetic operations
    mov eax, 10
    add eax, 20
    sub eax, 5
    inc eax
    dec eax
    
    ; Stack operations
    push eax
    push ebx
    pop ecx
    pop edx
    
    ; Logical operations
    and eax, 0xFF
    or eax, 0x100
    xor eax, 0x55
    
    ; Shifts
    shl eax, 2
    shr eax, 1
    
    ; Comparisons
    cmp eax, 50
    jg .greater
    jmp .done
    
.greater:
    mov eax, 100
    
.done:
    ; More operations to test buffer capacity
    mov ebx, 42
    add ebx, 10
    sub ebx, 5
    mov ecx, ebx
    add ecx, 20
    
    ; Final return
    ret

section .data
    value1: dd 0x12345678
    value2: dd 0x87654321
    counter: dd 0 
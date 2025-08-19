section .text
global _start
_start:
    mov eax, 2
    mov ebx, 0
    int 0x80
    ret
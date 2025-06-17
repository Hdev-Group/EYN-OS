bits 32 ; funny downgrade

section .text
        align 4
        dd 0x1BADB002
        dd 0x00
        dd - (0x1BADB002+0x00)

        dd 0
        dd 0
        dd 0
        dd 0
        dd 0 ; skip over load of flags

        dd 0 ; graphical mode
        dd 320 ; width of screen
        dd 200 ; height of screen
        dd 32 ; bit depth

global start
extern kmain ; kernel.c

start:
        cli ; clears interrupts 

        mov esp, stack_space
        push ebx
        push eax

        call kmain ; continue kernel.c kmain func
        call Shutdown
        hlt

section .bss
resb 8192
stack_space:

Shutdown: ; speaks for itself
    mov ax, 0x1000
    mov ax, ss
    mov sp, 0xf000
    mov ax, 0x5307
    mov bx, 0x0001
    mov cx, 0x0003
    int 0x15

WaitForEnter:
    mov ah, 0
    int 0x16
    cmp al, 0x0D
    jne WaitForEnter
    ret

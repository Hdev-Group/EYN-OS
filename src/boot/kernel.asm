bits 32 ; funny downgrade

section .text
        align 4
        dd 0x1BADB002
        dd 0x00
        dd - (0x1BADB002+0x00)

global start
extern kmain            ; kernel.c

start:
        cli             ;clears interrupts 
        call kmain      ;continue kernel.c kmain func
        call Shutdown
        hlt             ; halt cpu

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

; 32 bit ISR stub for syscall interrupt 0x80

bits 32

global syscall_entry
extern syscall_dispatch

section .text
syscall_entry:
    ; Save segments
    push ds
    push es
    push fs
    push gs

    ; Save general registers
    pusha

    ; Push synthetic error code and int number (0 for errcode, 0x80 int)
    push dword 0            ; err_code
    push dword 0x80         ; int_no

    ; Saved EDI is at [esp + 8]
    mov eax, esp
    add eax, 8 ; eax = &saved EDI (start of regs_t)
    push eax
    call syscall_dispatch
    add esp, 4

    mov [esp + 36], eax

    ; Pop our synthetic fields
    add esp, 8 ; discard int_no, err_code

    ; Restore registers and segments
    popa
    pop gs
    pop fs
    pop es
    pop ds

    iretd


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

    ; Load kernel data segment selector into ds/es for C code if needed
    mov ax, 0x10            ; assuming GDT: 0x10 = kernel data selector
    mov ds, ax
    mov es, ax

    ; Compute pointer to regs_t structure currently on stack at saved EDI
    mov eax, esp
    add eax, 36 ; eax = &saved EDI (start of regs_t)
    push eax
    call syscall_dispatch
    add esp, 4

    ; On return, EAX holds return value; store into saved EAX within pusha area
    ; Saved EAX is at esp + 8 (after int_no, err_code)
    mov [esp + 8], eax

    ; Pop our synthetic fields
    add esp, 8 ; discard int_no, err_code

    ; Restore registers and segments
    popa
    pop gs
    pop fs
    pop es
    pop ds

    iretd


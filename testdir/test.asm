; Simple test program for EYN-OS assembler
; Just returns immediately to verify basic assembly works
section .text
global _start

_start:
    ; Simple program that just returns
    ret

section .data
    ; Empty data section 
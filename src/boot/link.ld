OUTPUT_FORMAT(elf32-i386)
ENTRY(start)

SECTIONS
{
    .text :
    {
        *(.text)
    }

    .data : 
    {
        *(.data) 
    }

    .shellcmds :
    {
        __start_shellcmds = .;
        *(.shellcmds)
        __stop_shellcmds = .;
    }

    .bss  : 
    {
        *(.bss)  
    }
}

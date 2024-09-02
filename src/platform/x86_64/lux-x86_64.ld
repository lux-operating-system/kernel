OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

SECTIONS
{
    . = 0xFFFF800000200000;

    .text BLOCK(16) : ALIGN(16)
    {
        *(.text)
    }

    .rodata BLOCK(16) : ALIGN(16)
    {
        *(.rodata)
    }

    .data BLOCK(16) : ALIGN(16)
    {
        *(.data)
    }

    .bss BLOCK(16) : ALIGN(16)
    {
        *(.bss)
    }
}
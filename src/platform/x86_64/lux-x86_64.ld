OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

SECTIONS
{
    . = 0x200000;

    .text BLOCK(8) : ALIGN(8)
    {
        *(.text)
    }

    .rodata BLOCK(8) : ALIGN(8)
    {
        *(.rodata)
    }

    .data BLOCK(8) : ALIGN(8)
    {
        *(.data)
    }

    .bss BLOCK(8) : ALIGN(8)
    {
        *(.bss)
    }
}
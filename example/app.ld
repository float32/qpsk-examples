MEMORY
{
    RAM1 (rwx)       : ORIGIN = 0x20000000, LENGTH = 112K
    RAM2 (rwx)       : ORIGIN = 0x2001C000, LENGTH = 16K
    CCM (rwx)        : ORIGIN = 0x10000000, LENGTH = 64K
    FLASH (rx)       : ORIGIN = 0x08000000, LENGTH = 1M
}

SECTIONS
{
    .vtable (ORIGIN(FLASH)) : AT (ORIGIN(FLASH))
    {
        KEEP (*(.vtable))
        . = ALIGN (16);
    } > FLASH

    .text :
    {
        CREATE_OBJECT_SYMBOLS
        . = ALIGN (16);
        *(.text)
        *(.text*)
        . = ALIGN (16);
    } > FLASH

    PROVIDE (_etext = .);

    .rodata :
    {
        . = ALIGN (16);
        *(.rodata)
        *(.rodata*)
        . = ALIGN (16);
    } > FLASH

    .data :
    {
        _sidata = LOADADDR (.data);
        _sdata = .;
        . = ALIGN (16);
        *(.data)
        *(.data*)
        . = ALIGN (16);
        _edata = .;
    } > RAM1 AT> FLASH

    .bss :
    {
        _sbss = .;
        . = ALIGN (16);
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN (16);
        _ebss = .;
    } > RAM1

    .ram2 :
    {
        . = ALIGN (16);
        *(.ram2)
        *(.ram2*)
        . = ALIGN (16);
    } > RAM2

    PROVIDE ( end = _ebss );
    PROVIDE ( _end = _ebss );

    .ccm (NOLOAD) :
    {
        . = ALIGN (16);
        *(.ccm)
        *(.ccm*)
        . = ALIGN (16);
    } > CCM

    .stack (ORIGIN (CCM) + LENGTH(CCM) - 4 * STACK_SIZE) (NOLOAD) :
    {
        _sstack = .;
        . += 4 * STACK_SIZE;
        _estack = .;
    }
}

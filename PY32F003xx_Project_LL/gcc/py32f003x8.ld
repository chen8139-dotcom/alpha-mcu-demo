/*
 * py32f003x8.ld - GCC linker script for PY32F003x8 (Cortex-M0+)
 *
 * Ported from MDK-ARM/Objects/Project.sct:
 *   LR_IROM1 0x08000000 0x00010000  (Flash 64K)
 *   ER_IROM1 0x08000000, RW_IRAM1 0x20000000 0x00002000 (SRAM 8K)
 *
 * Difference from Keil: FLASH LENGTH is 0xFF80 instead of 0x10000, because
 * the application stores user NVM data in the last flash page
 * (0x0800FF80..0x0800FFFF, see user_flash_manage / user_bsp_flash).
 * The linker now enforces that program image never overlaps the NVM page.
 */

/* Entry Point */
ENTRY(Reset_Handler)

/* Keil startup allocated Stack 0x400 / Heap 0x0 as static areas. Under GCC
   the stack grows down from _estack, so only reserve 0x400 as a linker-time
   sanity check (not zero-initialized memory). */
_Min_Heap_Size  = 0x0;      /* heap amount = 0, same as Keil Heap_Size */
_Min_Stack_Size = 0x400;    /* required amount of stack = 0x400         */

/* Specify the memory areas */
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 0xFF80  /* 64K minus last page (NVM) */
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 0x2000  /* 8K */
}

/* Highest address of the user mode stack */
_estack = ORIGIN(RAM) + LENGTH(RAM);    /* end of "RAM" (0x20002000), same as
                                           Keil __initial_sp (Stack_Mem top)  */

/* Define output sections */
SECTIONS
{
  /* The startup code goes first into FLASH */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(4);
  } >FLASH

  /* The program code and other data goes into FLASH */
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.glue_7)         /* thumb->arm glue */
    *(.glue_7t)        /* arm->thumb glue */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;        /* define a global symbol at end of code */
  } >FLASH

  /* Constant data goes into FLASH */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    . = ALIGN(4);
  } >FLASH

  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH

  .ARM.exidx :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  .preinit_array     :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH
  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*))) KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH
  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*))) KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH

  /* used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* Initialized data sections goes into RAM, load LMA copy after code */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(.ramfunc)
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */

    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
  } >RAM AT> FLASH

  /* Uninitialized data section */
  . = ALIGN(4);
  .bss (NOLOAD) :
  {
    /* This is used by the startup in order to initialize the .bss section */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >RAM

  /* User_heap_stack section, used to check that there is enough RAM left */
  ._user_heap_stack (NOLOAD) :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}

__start = 0x80003100;
ENTRY(__start)

SECTIONS
{
  .init   0x80003100 : { *(.init) *(.init.*) }
  .text   0x800034A0 : { *(.text) *(.text.*) }
  /* .ctor/.dtor: split objects (include the null terminators); .ctors/.dtors: compiled objects */
  .ctors  0x8021C960 : { *(.ctor) *(.ctors) *(.ctors.*) *(.dtor) *(.dtors) *(.dtors.*) }
  .rodata 0x8021CA40 : { *(.rodata) *(.rodata.*) }
  .data   0x802511E0 : { *(.data) *(.data.*) }
  .bss    0x802704A0 : { *(.bss) *(.bss.*) *(COMMON) }
  .sdata  0x80313EE0 : { *(.sdata) *(.sdata.*) }
  .sbss   0x80315460 : { *(.sbss) *(.sbss.*) }
  .sdata2 0x80315CC0 : { *(.sdata2) *(.sdata2.*) }

  _SDA_BASE_ = 0x8031BEE0;
  _SDA2_BASE_ = 0x8032BEE0;
  _stack_end = 0x8031ACE0;
}

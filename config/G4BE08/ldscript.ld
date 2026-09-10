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
  /* Absolute symbols referenced by the Dolphin SDK objects (values from the original OSInit/OSThread code). */
  _stack_addr = 0x8031ACE0;
  _stack_end = 0x80316CE0;
  __ArenaLo = 0x8031AD00;
  __ArenaHi = 0x81780000;
  /* GX write-gather FIFO (include/gx.h declares it as an extern object). */
  GXWGFifo = 0xCC008000;
  /* MWCC (CRI) objects call the EABI helpers by register number; SN's eabi has only _savefpr_14/_restfpr_14 as symbols. */
  _savefpr_15 = _savefpr_14 + 0x4;
  _restfpr_15 = _restfpr_14 + 0x4;
  _savefpr_16 = _savefpr_14 + 0x8;
  _restfpr_16 = _restfpr_14 + 0x8;
  _savefpr_17 = _savefpr_14 + 0xc;
  _restfpr_17 = _restfpr_14 + 0xc;
  _savefpr_18 = _savefpr_14 + 0x10;
  _restfpr_18 = _restfpr_14 + 0x10;
  _savefpr_19 = _savefpr_14 + 0x14;
  _restfpr_19 = _restfpr_14 + 0x14;
  _savefpr_20 = _savefpr_14 + 0x18;
  _restfpr_20 = _restfpr_14 + 0x18;
  _savefpr_21 = _savefpr_14 + 0x1c;
  _restfpr_21 = _restfpr_14 + 0x1c;
  _savefpr_22 = _savefpr_14 + 0x20;
  _restfpr_22 = _restfpr_14 + 0x20;
  _savefpr_23 = _savefpr_14 + 0x24;
  _restfpr_23 = _restfpr_14 + 0x24;
  _savefpr_24 = _savefpr_14 + 0x28;
  _restfpr_24 = _restfpr_14 + 0x28;
  _savefpr_25 = _savefpr_14 + 0x2c;
  _restfpr_25 = _restfpr_14 + 0x2c;
  _savefpr_26 = _savefpr_14 + 0x30;
  _restfpr_26 = _restfpr_14 + 0x30;
  _savefpr_27 = _savefpr_14 + 0x34;
  _restfpr_27 = _restfpr_14 + 0x34;
  _savefpr_28 = _savefpr_14 + 0x38;
  _restfpr_28 = _restfpr_14 + 0x38;
  _savefpr_29 = _savefpr_14 + 0x3c;
  _restfpr_29 = _restfpr_14 + 0x3c;
  _savefpr_30 = _savefpr_14 + 0x40;
  _restfpr_30 = _restfpr_14 + 0x40;
  _savefpr_31 = _savefpr_14 + 0x44;
  _restfpr_31 = _restfpr_14 + 0x44;
}

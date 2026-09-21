# Naming: where every name in this tree comes from

Every identifier in `src/` and `include/` is in one of the classes below. The class decides how much
to trust the name and what evidence a rename needs (see `CONTRIBUTING.md`, "Proposing a rename").
Counts are from the date given next to them; the placeholder counts go down as fields are identified.

## Functions: the vendor's names, from the debug symbol files

The `G4BE08` discs are a debug build. They ship `files/Bio4.sym` for `main.dol` and one
`files/Bio4.<mod>.sym` per REL module. Those files list every function with its address, size and
name as Capcom's linker saw it. `config/G4BE08/sym_map.tsv` and `config/G4BE08/modules/<mod>/sym_map.tsv`
are those tables, one row per symbol: address, size, section, unit, scope, the linker symbol, the
readable name. Every function in the tree is defined under its row's name; the build fails to link
otherwise.

The linker symbols are C++-mangled in the GNU v2 scheme (`HermiteExport__13CameraControlP9CameraCutPUc`
is `CameraControl::HermiteExport(CameraCut*, unsigned char*)`). That is the direct evidence that the
game was written in C++ and compiled as C++: class names, member functions, overloads and parameter
types are all in the symbol table. The game code is `.cpp` for that reason. The units that are `.c` are
the ones whose symbols are unmangled C names: the newlib pieces linked into the DOL (`src/game/atof.c`,
`fopen.c`, ...), the Nintendo SDK (`src/lib/OS*`, `GX*`, ...), the CRI middleware (`src/lib/adx_*`,
`sfd_*`, `mpv_*`, ...) and SN's runtime. The exception is SN's libm (`src/lib/sf_sin.cpp`, ...): fdlibm
C sources included from a `.cpp` wrapper because ProDG compiled them through the C++ front end (the
wrapper's comment says why it matters for the data layout). Tree totals: 598 `.cpp`,
374 `.c`, 8 `.s` (2026-09-18).

Scope in the `.sym` (`local`/`global`) is not reliable and is corrected from use (`docs/matching.md`,
"Matching rules of thumb": "Bio4.sym scopes are unreliable").

Functions and labels without a vendor name keep a generated name: `fn_<addr>` for a code block the
`.sym` does not name (linkonce copies the module linker kept but did not list, see `docs/matching.md`
"Multi-object modules"), `lbl_<addr>` for an unnamed data object. 28 such names remain in `src/` and
`include/` (3 `fn_`, 25 `lbl_`; 2026-09-18, `rg -o 'fn_\w+|lbl_\w+' src include --no-filename | sort -u | wc -l`).
Do not replace them with invented names: a `lbl_` that is an unused `.sdata` global must keep its name for the
section layout to match (`docs/matching.md`, the rule of thumb on unused `.sdata` globals with no
`Bio4.sym` name).

## Files and unit boundaries: from the HALT strings and the symbol files

The vendor's source file names survive in the binaries. The `HALT`, `dbgAssert`, `MEM_ALLOC` and
`VECNormalize` macros expand `__FILE__`, so the `.rodata` of each object contains strings such as
`"D:/Bio4/Prog/em2d.cpp"`. A unit in this tree is named after that string (`src/em2d/em2d.cpp`).
Where one REL was linked from several objects, the
boundary between them is read from the `.sym` (function-name prefixes, the order of the header strings
each object emitted) and the HALT strings; `docs/matching.md`, "REL modules" and "Multi-object modules",
gives the method and the per-module results (for example `st1_0 = r100.cpp, r120.cpp, st1.cpp`).

Directory names are the module names from the disc: `em<xx>` enemies, `pl<xx>` players, `wep<xx>`
weapons, `st<n>_<m>` stages, `t_*`/`Tools` the debug tools, `Sscrn` the sub-screen. `src/game/` is the
DOL.

## `#line` directives

`#line 375 "D:/Bio4/Prog/cSceObj.cpp"` before a statement sets the line number the preprocessor
reports. The vendor's asserts, `HALT`s and `MEM_ALLOC` calls put `__LINE__` into strings and
immediates, and those values are part of the original bytes. Our source has different line numbers from
the vendor's, so each such site is preceded by a `#line` that restores the vendor's number and file
name. There are 685 of them in 262 files (2026-09-18, `rg -c '^\s*#\s*line' src include`). They are
not noise and must not be moved or removed. They also record roughly how long the vendor's file was and
where in it a function sat.

## Structs, fields, enums: three classes

The binaries have no type information for the game's data. Field names come from three sources, in
decreasing order of confidence.

### 1. Vendor names from the PS2 debug build

The PlayStation 2 port has a debug build dated Aug 28 2005 whose ELF carries a `.mdebug` section with
STABS type information: every struct, field, enum and function signature the PS2 compiler saw.
`stdump` from [chaoticgd/ccc](https://github.com/chaoticgd/ccc) turns it into C declarations;
`orig/ps2/ps2_types.h`, `orig/ps2/ps2_functions.h` and `orig/ps2/ps2_globals.h` are that output.
(`orig/` is not committed; the files are produced from your own copy of the PS2 ELF.)

The PS2 layouts do not match the GameCube layouts byte for byte: `Vec` is 16 bytes on the PS2 and 12
here, `Mtx` is 0x40 and 0x30, fields were added, removed and repacked in the port. So the fields are
matched by sequence, type and the names already known, never by raw offset. `tools/ps2sym.py` does
that: it aligns each GC struct against its PS2 counterpart (a sequence alignment scored on type
compatibility, existing-name equality and array dimensions, then a confidence from how well the
neighbours align once the `Vec`/`Mtx` growth is accounted for), writes a rename plan and applies it
to the declaration lines; the compiler then reports every use site, which the tool renames in turn.
Bytes never change under a rename; the clean rebuild check enforces that.

`python3 tools/ps2sym.py --struct <Name>` prints the alignment side by side (GC field, PS2 field,
confidence, reasons). A GC field with a PS2 field on its right at confidence 1.00 is a vendor name.
Where the vendor renamed a type between the ports, the alias table at the top of `ps2sym.py` says so
(`Em10Work` is the PS2's `FREE_EM10`, `MotionWork` is `MOTION_INFO`). Type names that appear in the
mangled GC symbols (`P10MotionWork` in `sym_map.tsv`) are GC vendor names and keep the GC spelling even
when the PS2 used another.

The tool aligns on names first, so a wrong name already in the header can pull a PS2 field onto the
wrong row. `Em10Work` had that: the `Vec` at 0x4EC, between `Keep_pos` and `Return_ck_pos` exactly as
the PS2's `Route_target` is, had been named `Goto_pos`, and the real `Goto_pos` (0x5F0, between
`Target_pos` and `Scale` as on the PS2) was a placeholder. When the row order around a field disagrees
with the tool's name match, the row order wins (2026-09-21: `Route_target` / `Target_pos` / `Goto_pos`).

Enums (`EM_STATUS`, `DATA_COMMAND`, `SCE_LEVEL`, ...) are from the same dump; where an enum was
introduced, the magic constants at typed call sites were replaced with its enumerators and the
parameter types left alone.

The pG flag words (`Debug_flg`, `Status_flg`, `Key_flg`, ...) are read and written through the
`XxxFlagChk/On/Off(pG, NAME)` macros of `global.h`; bit `no` is bit `31 - (no & 31)` of word `no >> 5`,
the numbering of the t_flag.cpp tables and of the PS2 enums (`DBG_FLAG`, `STA_FLAG`, `KEY_FLAG`, ...).
`Room_flg` has no global enum: the bits mean something different in every room. The five rooms whose
flags the PS2 build names (`enum ROOM_FLAG` blocks, matched to a GC room by the set of bits its code
uses) have an `enum R<xxx>_FLAG { RMF_... }` at the top of the room source, or in the header of the
object that shares the bits (`objRobo.h` r226, `em2b.h` r119, `pl14.h` r11c), and use `RmfFlagChk`;
every other room keeps the raw mask, since there is no vendor name to give the bit.

### 2. Names given from usage by the decompilers

Where the PS2 dump has no counterpart for a field (the struct was reorganised, or the field is
GameCube-only), the field was named from what the code does with it. These names are guesses. They
are recognisable in two ways: the field's line comment names the use (`cModelInfo* pRobe; // ...
type 6: body parts info (em10ModelInit)`), and `ps2sym.py --struct` shows nothing on the PS2 side of
that row. Treat them as a description, not a fact; a rename from better evidence is welcome
(`CONTRIBUTING.md`).

### 3. Placeholders

`xNN` is a field at offset `0xNN` from the start of its struct whose meaning is unknown (for an
overlaid work such as `Em10Work` the offset is work-relative and the comment gives the absolute one).
`pad_NN[k]` is `k` bytes at offset `0xNN` that no code we have read touches. 563 `xNN` fields remain in
`include/` (2026-09-18, `rg -c '\bx[0-9A-Fa-f]{2,4};' include | awk -F: '{s+=$2} END{print s}'`).
Names of this class carry no claim at all; the offset is the only information.

## Style: the vendor's spelling is kept

Names taken from the vendor are kept exactly as the vendor wrote them: `m_Pos`, `Wall_norm`, `Rno0`,
`be_flag`, `Timer2`, `TmpU32`. Capcom's own code mixes Hungarian prefixes, `Snake_case`, `camelCase`
and abbreviations; the mix in this tree is the mix in their source, not a style choice of ours. Renaming
a vendor identifier to a house style would destroy the one property the name has, which is that it is
the original.

Names we introduce (class 2) follow the surrounding struct: if the neighbours are `Timer`, `Timer2`,
`TmpF`, a new field is `TmpV`, not `tmp_vec`. Local variables in function bodies are ours unless the PS2
function signature supplied the parameter names (`ps2sym.py --params` renamed 434 functions'
parameters from it); they follow the same rule.

## `// COMPILER-DIFF:` tags and register pins

A `// COMPILER-DIFF: <n>` comment marks a construct that is there to make the compiler choose the same
register or schedule as the vendor's build, not to express program logic: a `register T x asm("rN")`
declaration, an empty `asm("")` with operand constraints, a dead test, a statement placed for its
side effect on the allocator. None of them emits an instruction. `python3 tools/asmcheck.py --all`
compiles every GCC unit with its asm templates marked and lists the instructions that came from a
template; the only hits are the paired-single, GQR, cache and exception-handler kernels the vendor also
wrote in assembly (231 instructions at 18 sites, unchanged since 2026-09-17). There are 644 tags and 122
`register ... asm("rN")` pins (2026-09-18). Each tag number is a mechanism explained in
`docs/matching.md` ("Known compiler-build differences", "Lever catalogue") and the pass-by-pass record
in `docs/research/`; `docs/research/compiler.md`, section "Asm-removal pass", records how the earlier hand-placed instructions were
replaced by C and which compiler pass each replacement relies on.

The CRI libraries (CodeWarrior) have the same thing in MWCC form: 27 codeless `asm { mr r11, x; mr x, r11 }`
and `asm { mr v, v }` blocks that the allocator deletes and that only narrow the register choice. One
block emits code: `dctac_Init` in `lib/dct_ac.c`, where the vendor's compiler build pooled the
function's 8-byte literals differently from ours (`docs/matching.md`, "MWCC compiler-build differences").

## Where to look

- `config/G4BE08/sym_map.tsv`, `config/G4BE08/modules/<mod>/sym_map.tsv`: the vendor's function names.
- `python3 tools/ps2sym.py --struct <Name>`: which fields of a struct are vendor names, which are ours.
- `python3 tools/ps2sym.py --summary`: per-header counts, structs with no PS2 counterpart.
- `docs/matching.md`: unit boundaries, tags, conventions. `docs/unit-notes.md`: per-unit notes.

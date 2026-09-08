# Resident Evil 4 (GameCube, G4BE08 debug build) — matching decompilation

Goal: C/C++ source that compiles to a byte-identical `main.dol`. The build already reproduces the
original DOL from split objects; every unit you match replaces one split object with compiled code.

## Facts you need

- Compiler: **SN Systems ProDG (GCC 2.95.x)**, not CodeWarrior. Default toolchain `ProDG/3.9.3`,
  flags `-O2 -mfast-cast` (see `configure.py`; `-mps-float` is wrong: 16-byte FPR save slots). Game code is C++ (old GNU v2 name mangling,
  e.g. `Comeback__13CameraControl`). Some `game/` units are actually newlib C (strlen, atoi, vprintf...).
- Symbols: `config/G4BE08/sym_map.tsv` — address, size, section, unit, scope, current symbol name,
  original demangled name (from the debug build's `Bio4.sym`). Function boundaries and sizes are exact.
- Target asm per unit: `build/G4BE08/asm/<unit>.s` (dtk disassembly with symbolic relocations).
- Types: `include/types.h` (u8..f64). Shared class/struct definitions go in `include/<name>.h`;
  check existing headers before adding a type, and only extend, never rewrite, structs other units use.
- Globals seen via `r13`/`r2` (`@sda21`) are small-data; declare them `extern` with the exact symbol name.

## Workflow for one unit (`game/foo`)

```sh
python3 tools/unit_info.py game/foo            # functions, sizes, demangled names, current match %
sed -n '/^\.fn NAME/,/^\.endfn/p' build/G4BE08/asm/game/foo.s   # asm for one function
# write src/game/foo.cpp (or .c for the newlib/C units), then:
ninja build/G4BE08/src/game/foo.o              # compile (errors are printed)
python3 tools/sync_symbols.py build/G4BE08/src/game/foo.o   # renames placeholder symbols to the mangled names
python3 configure.py && ninja                  # rebuild, refresh report (must still say main.dol OK)
python3 tools/fdiff.py game/foo <mangled_symbol>   # side-by-side diff of one function (only differing lines; --all for everything)
```

Repeat until every function in the unit is 100%. Then set `MATCHING["game/foo.cpp"] = True` in
`config/G4BE08/objects.py` (create the dict if missing), run `python3 configure.py && ninja`, and
confirm the SHA-1 check still passes (`ninja` prints `build/G4BE08/main.dol: OK`). If the link fails
after linking a unit, its data/rodata layout differs from the original: fix the source, do not
mark it Matching.

## Matching rules of thumb (GCC 2.95)

- ProDG has no header dependency tracking: after editing a header, `touch` the sources that include it.
- `size_t` is `unsigned int` (see `include/newlib_local.h`); `unsigned long` changes register allocation.
- `include/global.h` has `pG` (`GlobalWork`) and the `BitOn(u32&, bit)`/`BitOff` helpers: the original
  sets/clears flag bits through a reference, which makes GCC reload `pG` after the store and keep
  consecutive `|=` separate. Use them where the asm shows that pattern.
- `include/dolphin/*.h` are CodeWarrior-only (SDK units); game code uses `include/vec.h` for Vec/Mtx/PS*.
- Loops that search and set: `for (...) { if (hit) { ...; break; } }`; a `return` inside the loop gives a different tail.
- Zeroed local arrays (`T* a[3] = {NULL, NULL, NULL}`) become a `memset` libcall with `crclr cr1eq`.
- GCC 2.95 puts vtables of classes without a key function and template/inline instantiations in
  `.gnu.linkonce.*` sections; `tools/fold_linkonce.py` (post-build for all `game/` objects) folds them
  back the way the original linker did. Classes are declared in `include/cManager.h` (`cUnit`,
  `cManager<T>`), `model.h` (`cCoord`, `cModel`), `em.h`, `obj.h`, `esp.h`, `light.h`.
- `cUnit::operator delete` takes `unsigned int`, not `u32` (`u32` is `unsigned long`).
- Header-owned strings (`cManager` messages, `__FILE__` from inline range checks via `#line`) appear in
  each unit's `.rodata`; unused inline functions still emit their strings.
- Field names must be agreed across units: a header field may only be renamed if you update every user.
- objdiff's 100% only covers functions present in the target; **also check the compiled object's `.text`
  size equals the split object's** (`dtk elf info build/G4BE08/src/<unit>.o` vs `build/G4BE08/obj/<unit>.o`).
  ProDG emits every in-class member function body out of line (to `.text`, not linkonce), so an inline
  member the original never had grows `.text` and breaks the DOL even though objdiff reports 100%.
- Fast-cast GQR mapping: explicit `(f32)` of a u8/u16/s8/s16 load → `psq_l` with qr2/qr3/qr4/qr5; f32→u8
  stores use `psq_st qr2`; a value already in a register goes through the `0x43300000` double trick.
- `x + -1.0f` gives `fadds -1.0`; `x - 1.0f` gives `fsubs`. `no / per` and `no % per` share one `divwu`.
- Two consecutive `|=` on a u8 field merge into one store unless another store sits between them.
- Stack slot order follows declaration order; float register assignment of same-lifetime locals depends on
  declaration order. `for (i = n - 1; i >= 0; i--)` → `subic.`/`bge` loop.
- Zero-initialised static locals go to `.sdata`, uninitialised to `.bss`; `static const` locals are named
  symbols; anonymous constant-pool aggregates come from non-static constant expressions.
- A block-local `Work* w = &work;` inside the branch vs at function top decides whether the `addi` is hoisted.
- `pLog` is a `cLogPtr` struct wrapper (`include/db_log.h`): loading it as a struct member stops the
  scheduler hoisting the load above stores through `this`. GX FIFO writes go through
  `((GXHwRegs*)0xCC000000)->wgpipe` (`include/gx.h`), which gives `lis rX,0xCC01` + `-0x8000(rX)`.
- Each extra int→float conversion in a block leaves a dead `mr` between two unused GPRs; a call or
  `asm("")` between them prevents it.
- Loop shapes: `if ((v = x) == 0) { do {...} while ((v = x) == 0); }` duplicates the entry test;
  `for` + `break` gives `cmpwi`/`bgt` without ctr, `return` in the body gives `bdnz`.
  `for (w = wk, i = 0; ...; w++, i++)` vs separate init changes callee-saved register choice.
- A loop-invariant `&Global` held in a register (`addi r31,...,sym@l`) means the source used a pointer
  variable (`MessageControl* m = &cMes`); a two-step `addi rX,rX,sym@l; addi rX,rX,4` comes from an inline
  accessor returning `&this->member`.
- Header-owned strings and initializer templates of unused inline functions are emitted in parse order;
  all-zero aggregates emit nothing; constant pools are emitted at each function's end (`.rodata`
  interleaves them).
- Bio4.sym scopes are unreliable: symbols marked `local` in `sym_map.tsv` are often global (called from
  other units) — check callers' asm before making them `static`.
- Static locals show as `name.NNN` in objdiff; the DECL_UID suffix cannot be reproduced and is ignored by the report.

- Struct field offsets come from the load/store displacements; write real structs, not casts.
- `rlwinm rX,rX,0,MB,ME` with wraparound = `x &= ~bit`; `ori` = `|= bit`.
- Branch shape follows the source: `if/else if` chains vs `switch` produce different compare orders
  (see `AreaCheckOnOff` in `src/game/cam_ctrl.cpp`: the original is a `switch`).
- Return `u8`/`s8` fields: `lbz` alone = unsigned, `lbz`+`extsb` = signed.
- Do not add casts to raw offsets to force codegen. Do not rename globally visible identifiers away
  from the names in `sym_map.tsv`.
- Data (`.rodata`/`.data`/`.bss`) in your unit must also match: define the globals the unit owns
  (see `[.data]`/`[.bss]` in `unit_info.py` output) with the right sizes and initial values.

## Don'ts

- Never edit `build/`, `build.ninja`, `objdiff.json`, or `config/G4BE08/splits.txt` by hand.
- Do not run interactive `objdiff-cli diff`; use `tools/fdiff.py` (one-shot).
- Do not commit; the orchestrator commits.

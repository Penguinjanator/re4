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
## Compiler version

The pack has five ProDG cc1plus builds (3.5/3.5b140 = GCC 2.95.2 SN v1.40, 3.7 = v1.46, 3.8.1 = v1.55,
3.9.3 = 2.95.3 SN v1.76). All five emit byte-identical `.text` for esp0f/esp16/esp01/esp15, including the
still-unmatched functions (dead `mr` from fpmem sharing, `0.0f + z` folding, `lfs; fmr` for `t = 0.0f`).
So a remaining diff is never explained by "compiler version" with the tools we have: keep looking for a
source form. (Older assemblers reject `ldh`; 3.9.3 is the build.) The DOL has no compiler string; its
GCCI is "Ver.1.09 Build Oct 8 2004".


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
- Stores through raw pointers/references (non-struct MEMs) make GCC reload `pG` afterwards and keep
  loads in source order; struct-member stores don't (`TOOL_FLAG` raw-offset accessors in `t_util.h`).
- Uninitialised globals are emitted in order of *first declaration*, header externs included, so header
  extern order dictates `.bss`/`.sbss` layout; initialised objects are emitted at their definition.
  Statics/initialised globals ≤ 8 bytes go to `.sdata` (-G 8); 16-byte zero-initialised objects to `.data`.
- `__attribute__((aligned(32)))` buffers create the 0x10 `.bss` holes; a 32-aligned following unit leaves
  `.sdata` padding the split objects don't have (`t_util.cpp` pads with `asm(".section .sdata; .balign 32")`).
- Clamps: `x = x < 0 ? A : (x > B ? 0 : x)` → li/mr chain, one store; `if (v >= 0) { n = v; if (n > M)
  n = M; } else n = 0;` → blt/li-at-end. `if (c) { ...; return X; } rest; return Y;` lays out `rest` first.
- Register args are evaluated left to right and kept live across a nested call in the argument list;
  originals often compute the inner call into a local first.
- `memcpy(dst, "literal")`/`strcpy` with a constant source inlines to word/byte moves; `char s[64] = ""`
  → `lbz` + `memset(s+1, 0, 63)`; `Vec v = {0,0,0}` inside a loop → `memset` per iteration.
- The original never moves a load of a global (`pG`, a static float) above a store made through `this`
  or a member pointer; ProDG does unless the store goes through a scalar reference — `FSet(f32&, f32)`,
  `BitOn16(u16&, u16)` in `include/global.h` reproduce the original order. No compiler flag changes this.
- `fabsf` is a volatile asm (`include/math_sub.h`) and acts as a scheduling barrier.
- Frame layout: `Vec`/`Mtx` locals are 8-byte aligned; function-level locals in declaration order from
  0x8, block-scoped ones after the block's temporaries, freed block slots reused — block scoping matters.
- GCSE: an expression used in both `if/else` arms and after the join must be written inline, not
  pre-computed into a variable. `ret = f(); ...; return ret;` in every branch stops cross-jumping of
  identical call tails.
- `SetFreeWork(EspGenWork*, u32* seed)` is the real cEsp virtual signature (seed in r5).
- GCC 2.95 emits *every* in-class inline member of a class whose vtable is emitted in the TU (key
  function defined here), used or not. Unused inlines of non-polymorphic classes and unused free/static
  inline functions are not emitted, but their string literals and float constants still land in
  `.rodata`. So: stray constants/strings in the target `.rodata` with no body → an unused inline of a
  non-polymorphic class or a free inline, never an extra member of the polymorphic class.
- `switch` on a `u8` with `case 0:` sharing the default body: `cmpwi 2; beq; ble default; cmpwi 3; ...`.
- Chained `a = b = c = 0` shares one zero register across `stw`/`stb`; separate statements get their own.
- `#line N "D:/Bio4/Prog/<unit>.cpp"` before `MEM_ALLOC` reproduces `__FILE__`/`__LINE__` strings.
- ProDG's scheduler ranks ready insns by register pressure before priority; no cross-block hoisting inside
  functions with loops.
- A global pointer loaded twice around a block copy: a plain `T* pT` gets the second load hoisted; reading
  it as a struct member (`((Wrapper*)&pT)->p`, like `cLogPtr`) keeps it below the stores.
- `for (...) { if (hit) { call; return; } }` duplicates the loop entry test; with `break` the loop is
  rotated with a single bottom test. `if (a && b) {reset} else {load}` vs the swapped form controls
  branch layout.
- Unused `static const` scalars are emitted (`.sdata2`) only when an emitted function takes their address;
  zero-initialised statics referenced only by a never-called inline are still emitted. Static locals are
  emitted at their declaration; file-scope uninitialised statics after all function-local ones.
- `s16 mem += (int)(s16)(float)` keeps `lha/extsh/add/sth`; without the `(int)` cast the front end
  narrows to u16 arithmetic. A local `int num = 5` divisor gives `divw` by register, not the magic multiply.
- Struct-member view of a global pointer (`pGS`, `pEffParentWorldS`, `pLog`) keeps its load after a
  preceding store through `this`; but wrapping the global's *declaration* reorders loads in units that
  already match, so use the view macro only where the target shows it.
- Independent stores at a block end are issued in reverse RTL order (`a = b = c = 0` -> reverse;
  separate statements `x; y; z` -> `z, x, y`). CSE reuses the newest register holding a constant.
- A reload of a just-stored member is forwarded as `mr`; a local gives no copy. `if (c < n) x = c+1;
  else x = n;` yields an `mr` before the compare; `x = n; if (...) x = ...` loads into `x` directly.
- `T* p = alloc(); g.p = p; p->init();` gives `mr r0,r3; stw r0`; assigning the call result directly
  stores r3. Loading a wrapped global once into a local avoids per-store reloads.
- `!(flag & 1)` as an `if` condition gives `xori; andi.` when the same test exists in two cross-jumped
  branches; `(flag & 1) == 0` gives a plain `andi.`.
- `*(u32*)(char_ptr + i*4) = 0` keeps the pointer `lwz` inside a `bdnz` loop (may alias) and prevents
  loop reversal; a typed array store gets hoisted and the loop reversed.
- `va_list`: include/va_ppc.h; `va_start` is a struct copy in C++ (g++ 2.95 does not inline
  `__builtin_memcpy`).
- `.sdata` alignment padding a split object contains is reproduced with
  `asm(".section .sdata; .balign 8")`.
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

- Never run `git stash`, `git checkout -- <file>`, `git reset` or anything else that rewrites the shared
  working tree: other agents are editing it at the same time.

- Never edit `build/`, `build.ninja`, `objdiff.json`, or `config/G4BE08/splits.txt` by hand.
- Do not run interactive `objdiff-cli diff`; use `tools/fdiff.py` (one-shot).
- Do not commit; the orchestrator commits.

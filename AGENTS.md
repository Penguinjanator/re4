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
## Compiler

The compiler proper is a **native Linux `cc1plus`/`cc1` built from SN's GPL source drop, "2.95.3 SN BUILD
v1.79 for Nintendo Gamecube"** (`build/compilers/ProDG/3.9.3-v1.79/`, untracked build output). SN's
`cpp.exe` and `NgcAs.exe` from the 3.9.3 pack still run through wibo; `tools/ngccc.py` chains the three
exactly like `ngccc.exe -v` shows (same cpp defines, `-G 1024` → `-G1024`, `-D__OPTIMIZE__` only for
-O>0, `LANG=C` for the lexer). `configure.py --prodg-driver native` (default) selects it;
`--prodg-driver ngccc` is the old `ngccc.exe` (cc1plus v1.76) path. Rebuild the binaries with
`tools/sn-gcc/build.sh` with `SN_GCC_SRC` pointing at the extracted NGC_GNU_SRC/NGC source drop (copies the drop, CRLF→LF, `patches/linux-host.patch`
= i386 Linux host config + a dangling-pointer fix in `cp/decl.c`, `make -m32 -static`, installs into
`build/compilers/ProDG/3.9.3-v1.79/`; ~5 s). Why: the pack's five cc1plus builds (3.5/3.5b140 = GCC 2.95.2
SN v1.40, 3.7 = v1.46, 3.8.1 = v1.55, 3.9.3 = 2.95.3 SN v1.76) all share one fpmem-address unspec
between the GQR fast-cast conversions and the classic double-trick ones, so `reload_cse_regs` emits
cross-kind `mr` copies the original never has (see "Dead `mr`" below). v1.79's `rs6000.md` uses unspec 17
for the fast-cast family and 11 for the classic one, which is what the original binary does: on all 224
ProDG units the v1.79 build produces byte-identical objects to v1.76 except the 16 that mix the two
conversion kinds, and there the extra `mr`s disappear (Filter07/09/0bGXDraw, fadeDraw,
Draw_line3d_local_222, Esp11_SetParam, EspStrip_draw_poly, Light02/05/06_Move went to 100%). A remaining
diff is therefore never "compiler version": keep looking for a source form. The DOL has no compiler
string; its GCCI is "Ver.1.09 Build Oct 8 2004".


## Per-unit compiler flags

Not every game unit is `-O2`. The sound driver (`snd_iss*/seq*/str*/sub*/main/efx/ram`) is C++ with
`extern "C"` linkage compiled at **-O0** (frame pointer in r31, every local in a stack slot, args reloaded
before every use). `config/G4BE08/objects.py` has `UNIT_CFLAG_OVERRIDES` (flag -> replacement per unit).
If a unit's functions all start with `stwu; mflr; stw r31; mr r31,r1` and reload parameters from the
stack, suspect -O0 before spending time on -O2 forms. snd_drv.h/snd_sdk.h hold the driver types and
the trick for pulling in SDK headers under ProDG.

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
  scheduler hoisting the load above stores through `this`.
- GX FIFO: `GXWGFifo` is a linker-provided absolute symbol (`extern volatile WGPipe GXWGFifo[]` in
  `include/gx.h`, `GXWGFifo = 0xCC008000` in `config/G4BE08/ldscript.ld`), so the stores are
  `lis rX,GXWGFifo@ha` + `GXWGFifo@l(rX)` (same bytes as `lis 0xCC01`/`-0x8000`). objdiff shows the
  reloc as ARG_MISMATCH against the split object; the report and the linked DOL are what count. The
  address must be a SYMBOL_REF: with a constant address (`*(volatile WGPipe*)0xCC008000`, a struct
  member at 0xCC000000, or a `const` pointer) the scheduler issues the `lis/lfs` of `Screen` and
  the constant pool before the FIFO `lis`, and `*(volatile WGPipe*)0xCC008000` even gives `lis`/`ori`
  into a base register. An 8-byte `extern volatile WGPipe GXWGFifo;` lands in small data (`@sda21`),
  hence the incomplete array.
- Dead `mr rX,rY` between unrelated GPRs around `-mfast-cast` conversions are copies of the fpmem
  (stack slot) "address" pseudo: every conversion is split before sched1 into `loadaddr` (emits no
  code) + store + load with a scratch register; after reload, `reload_cse_regs` replaces a later
  `loadaddr` with a copy of whichever hard register still holds the previous one (forgotten at a
  code label, at a call, or when that register is overwritten; deleted when both got the same
  register). sched1 always hoists a `loadaddr` (no inputs, long chain through the store/load) to
  the first free integer slot of the block, so two conversions in one block overlap and get
  different scratch registers: a copy is inevitable unless a call, a label or an overwrite lies
  between them. Same-kind copies are in the original too (`mr r9,r11` in Filter07GXDraw, three in
  matched esp4c `move`).
  **Cross-kind copies (fixed by the v1.79 compiler, see "Compiler"):** the original compiler keeps TWO
  address values — one for the
  classic fpmem conversions (`stfd/lwz` fix, `stw/stw/lfd` float: `fix_truncdfsi2`/`floatsidf2`) and
  one for the GQR fast-cast ones (`psq_st`+`lbz/lhz`, `stb/sth`+`psq_l`: `fixuns_truncsfqi2`,
  `floatqisf2`, `floathisf2`, ...). It never copies across the two kinds; the shipped cc1plus v1.76 (all
  five ProDG builds, with `-mfast-cast`, `-mps-nodf`, `-mps-float`, `-msafe-sda`) emits one
  `(unspec [(const_int 0)] 11)` for every conversion type, so `reload_cse_regs` also copies psq↔classic;
  the v1.79 source build uses unspec 17 for the fast-cast family and matches. Evidence: every extra `mr`
  in Filter07/09/0bGXDraw (4 each), fadeDraw (u16 `psq_l` → int magic), esp15 `move`, esp19
  `Draw_line3d_local_222`, esp11 `Esp11_SetParam` is a cross-kind copy, and in esp11/esp19 the target
  shows the two chains directly: `mr r11,r10; mr r8,r10; mr r7,r10` (int→f32) interleaved with
  `mr r3,r5; mr r30,r5; mr r29,r5` (f32→u8) where r5 is a fresh `loadaddr` although r10 held the
  address. No matched unit has a cross-kind copy. Classify with the `.greg` dump: a `movsi` whose
  source register was last used by a `*_store1/_store/_load` of the other kind. Source forms cannot
  change the unspec (statement order, u8/int/u32/s16 at the conversion, `(u8)(f64)`, locals before
  the `if`, u8 helper params, direct FIFO stores — all tried); with the v1.79 build these functions
  match. If you still see a cross-kind copy, check that `build.ninja` uses `tools/ngccc.py` (run
  `python3 configure.py`). `rnd Rnd` (`clrlslwi`/`mr r0,r9`) is a different, plain
  register-allocation diff.
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
- `subfic r0,rX,0; adde r3,r0,rX` is `return x == 0`; `return x != 0` compiles to a branch.
- A `cmpw rCONST,rX` with the constant hoisted into a callee-saved register is a local like
  `int dead = 10;` compared as `dead < x`.
- A `do { ... } while (0)` macro body is a scheduling-region boundary: its stores do not mix with the
  preceding block's stores.
- Hardware registers are struct members at a base (`OS_BUS_CLOCK`: `lis 0x8000; lwz 0xF8(r)`), never
  `*(u32*)0x800000F8`; the GX FIFO is the linker symbol `GXWGFifo` (see gx.h).
- A void-looking function whose last call's r3 is untouched may return that value.
- libm/libc functions (`tanf`, `sqrtf`, `memcpy`...) must be declared `extern "C"` (fdlibm.h / the
  newlib headers). A C++-linkage declaration makes the unit reference `tanf__Ff`; sync_symbols now
  refuses to rename the C symbol and tells you.
- A float variable assigned twice in one block never gets its chain tied by local-alloc (intermediates
  in f0); one variable per chain gives the tied form. A variable used in two loops is globally allocated;
  declare it inside each loop body for separate pseudos.
- Unused `static const` arrays inside a function are still emitted before that function's pool;
  file-scope unused statics are dropped; <=8-byte objects go to `.sdata2`. `int x = 0;` at file scope
  lands in `.sdata`, not `.sbss`.
- cse canonical register: for `(set v x)`, `v` replaces `x` in the extended block only if `v`'s last
  mention is later than `x`'s; otherwise `x` stays canonical and both live (`fmr`). A dead trailing
  reload/copy (`p = pn;` after a loop) changes which one is canonical and thus the copy shapes.
- `while (v < bound) v += step;` recomputes `bound` per iteration; `do/while` or `for` hoists it.
- A `memcpy` whose destination is byte-pointer arithmetic (`(u8*)w + ofs`) keeps a following `.sdata`
  load below the stores; `(u8*)&w->v` casts are stripped by the builtin and behave like a struct copy.
- Identical local aggregate initializers in different functions are merged into one `.rodata`
  template; if the original kept one per function, give each a distinct type.
- Sibling blocks reuse freed stack slots first-fit; a nested block inside a live variable's block does
  not. Reassigning a pointer local after calls forces it into a callee-saved register.
- OPEN (id_sys `setCk`/`dispSw`/`kill`): the target zero-extends one `u8` parameter (`clrlwi rX,rParam,24`)
  before using it as a bit-table index while other u8 params are never masked; no source form found yet
  (u8/int/u32 locals, casts, `& 0xFF`, inline helpers, references, bitfields all tried).
- `if ((p = f()) != 0) {A} if (!p) {HALT}` keeps the compare in cr4 (`mcrf cr4,cr0`); two plain `if`s
  let cse fold the second.
- SN patched out jump tables: 4+ dense cases still give a compare tree; a `case` whose body is only
  `break` still counts as a tree node; writing `default:` first lays the default body out first.
- A local `arc = pG->pPlArc` reused across blocks becomes a global pseudo and ties pG's register; the
  original often re-reads `pG->field` per call. A function-scope `void* p = 0` in a callee-saved register
  gets reused by cse as the constant 0 argument of later calls.
- Store-order rule (sched1): in a block of independent stores, stores whose source register dies
  (last use) are issued first in RTL order, then the non-dying ones in RTL order. A constant shared by
  several stores has one pseudo, so only its last store is a death; a value reused after a branch
  sinks to the end of the block. Derive the source order from the target with this rule instead of
  permuting blindly.
- haifa tie-break: after a non-void call, the next call's arg `li`s outrank the `mr r3,this` copy;
  declaring the callee `void` when the original ignores its result changes the order.
- objdiff scores 100% even when constant-pool *values* differ (relocs compared symbolically): always
  cmp .rodata bytes against the split object before flipping a flag.
- In-class inline members of the class whose vtable the unit owns are emitted after the destructor at
  the end of `.text`.
- Interblock scheduling is on: an independent `i++` in a loop's join block is hoisted into the loop
  header unless `i` is used inside the diamond.
- A store through a plain pointer variable (`*d = v`, no `+` in the address) is assumed to alias
  `static` scalars and forces their reload; `p[i] = v` / `a->p[i].x = v` never aliases a fixed scalar.
- A block ending in a call followed by a label gets a nop that blocks cross-jumping into its tail; a
  dead trailing statement suppresses it and the tails merge one insn deeper.
- Narrow zero stores reuse the nearest wider zero pseudo (HI before SI); the SI zero for pointers
  stays separate.
- OPEN (mes `move`/`WidthCk`): `code = f(x); if (code == 0)` — original keeps `mr r4,r3; cmpwi r4,0`
  where ours combines to `mr. r4,r3`; ~20 forms tried.
- `union { GXColor c; u32 w; } kc; kc.w = 0x6600FF32;` gives the `lis/ori/stw` word store for colour
  constants; a `GXColor k = {..}` initializer gives per-byte `stb`s.
- An 8-byte member anywhere in a class (`u64`) raises the object's `.bss` alignment to 8, creating the
  unnamed 4-byte gaps dtk labels as separate symbols.
- gcse PRE of a struct load across an if/else is blocked by any memory kill in the arm: if the target
  lacks a PRE shape ours produces, the original arm stored to memory somewhere.
- Every store to a `GlobalWork` field followed by a `pG` reload = the original stored through
  reference setters (`U8Set/U16Set/U32Set`, `BitOn16`), not plain member stores. Four byte stores after
  a call through one fresh pointer load = an inline with its own `cPlayer* p = pPL` local.
- A `li rZ,0` in both arms of an if/else whose common tail follows = source-level tail duplication
  (the rest of the function repeated in each arm; jump2 cross-jumps the suffix).
- `x == 2 || x == 3` on a u8 returns as `subi 2; subfic 1; li 0; adde` (unsigned `<= 1` range fold).
- Loop-invariant `cmpwi cr2/cr3/cr4` hoisted before a loop = a `switch (type)` inside the loop body.
- Dead strings: code under `if (0)` or after `return` still emits its string literals into `.rodata`.
- Zeroed aggregate initialisers (`u16 h[3] = {0,0,0}`, `Vec a = {0,0,0}`) become `memset` libcalls
  with `crclr cr1eq`.
- SF constants have a tied GPR/FPR class: when a callee-saved GPR is free, the constant may land in the
  GPR and be stored with `lwz/stw`.
- Row-by-row matrix copy: `while (i--) { dp = *d; sp = *s; for (j<4) *dp++ = *sp++; s++; d++; }`
  gives the `cmpwi -1` reversed outer loop. Byte assembly through a `union { f32 f; u8 b[4]; }` gives
  the `rlwimi` chain.
- An inline `if (c) return 1; return 0;` materialises `li 0; bge; li 1; cmpwi`; `return c` gives
  `mfcr/extrwi`.
- Vtable emission is in reverse class-declaration order; a vtable that needs an uninstantiated
  template member instantiates it in place (strings between vtable groups).
- Weak vtable copies: a later unit's reference binds to the first copy program-wide; symbols.txt must
  name the first copy `_vt.<Class>` and the others `<Class>_virtual_table_<addr>`.
- `extern "C"` functions with function-pointer parameters need `extern "C"` on the definition too.
- Declare each C function in exactly one header (its owning unit's); a second declaration with a
  different signature in another header is a compile error the moment both get included.
- Placeholder names in sym_map.tsv for functions of unmatched units carry no linkage information;
  only already-mangled entries prove C++ linkage. Check a caller's `bl` in the asm when unsure.
- Cross-jumping merges a case body with the fall-through code only as far as it matches, then
  retargets the jump to a new label; jumps to that new label are never merged again. Default-equal
  cases must come first in source order.
- `static const Vec` locals declared mid-function are emitted into `.rodata` at that point and passed
  by address; non-static `const Vec` locals are copied to the stack.
- A switch whose body is a dead local store keeps its compare instructions (branches removed after
  flow deleted the store): `cmpwi ...; b L` sequences with unused compares.
- Narrow values (u8/u16/s16) masked at call sites or after entry (`clrlwi rX,rY,24/16`, `extsh`) where
  ours treats them as already extended: a u8/u16 struct member read *directly* in two places, the first
  in a QI/HI-mode context (range fold `x >= 0xF8 && x <= 0xFD`, a `switch`), stays a QI/HI pseudo and
  every later int use gets an explicit mask; a local `u8 v = p->member` is promoted and never masked.
  Likewise `u32 no = rec->x6; if (no > 0x7F)` keeps `cmplwi 0x7f` where a u8 local folds to `andi.`.
  Try this first on the id_sys/emobj/em_set OPEN cases.
- `int susp = !(m->flag & bit); if (susp) return;` gives `xori; andi.; bne`; the direct
  `if (!(x & bit))` gives plain `andi./beq`.
- A single `return ret` reached by `goto ok` from several paths keeps `li r31,1` + `mr r3,r31`;
  separate `return ret` statements are constant-propagated to `li r3,1`.
- A global read after a store through an out-pointer is loaded first only if copied into a local
  before the store. `u32 max = f(); if (w->id < max)` loads `id` after the call; `w->id < f()` keeps
  `id` in a callee-saved register across it.
- Zero-initialised function-pointer table `= { NULL }` lands in `.data`; uninitialised in `.bss`.
- Byte stores alias everything (alias.c: QImode store -> every global reloaded). Word/half stores
  through a varying struct pointer never alias a fixed scalar; if the target reloads a global pointer
  after such a store, the original stored through a scalar reference (`PSet(void*&, void*)`).
- A sum written as two statements (`d = a*b + c*d; d += e*f;`) keeps the intermediate in the
  variable's register instead of a temp tied to a dying operand.
- An unused aggregate local still takes its frame slot.
- KNOWN DEBT: cModel's real size is 0x320 (ctor initialises up to 0x31C: motion @0x1D8, cAtariInfo
  @0x2B4 ...) but em.h/obj.h currently define those fields inside cEm/cObj. game/model needs the fields
  moved into cModel with cEm/cObj starting at 0x320 — a coordinated refactor, do not start it while
  em*/obj* agents are running.
- Callee return type changes arg-setup order through dependence counts: an `int` result adds an
  output dependence on r3 that pulls `li r3,0`/`mr r3` to the end of the arg block. Declare the callee
  with its real return type (check the callee's own asm); `EstSet`, `MotionSetCore` are `void`.
- Store-block weight rule (generalises the store-order rule): each independent store has weight +1,
  minus 1 per source register that dies there; lowest weight first, ties in source order.
- `x >= C && x <= C` on a float yields the `cror un,eq,gt / bns` pair.
- `for (i = 0; i < 8; i++) Wk[i].flag = 0` compiles to a `mtctr 8` loop stepping the pointer down
  from the last element.
- A loop-invariant `li rX, mask` left inside a small loop = several separate `&= ~bit` statements
  merged by combine after loop opt.
- Fresh block-local pointer copies in a later section (instead of reusing function-level pointers)
  shorten live ranges and re-rank callee-saved register assignment.
- cModel size debt workaround: an unused `u8 pad[0x320 - sizeof(cModel)]` after a `cModel` local
  reproduces the original frame footprint.
- `*(u32*)((u8*)p + ofs)` (cast then deref) produces a MEM without `MEM_IN_STRUCT_P`, so it aliases
  scalar globals and forces `pG/pSys/pRK` reloads; `p[i]` / `*(p + i)` does not.
- `if (A || B) return 0;` places the `li r3,0` block after the second test; separate `if`s after the
  first. `!(d >= 0.0f)` and `while (!(x <= 0.0f))` produce the `cror` form; `<`/`>` plain `bge/ble`.
- ngcld does not honour a following unit's 32-byte `.bss` alignment: when the original unit's `.bss` is
  larger than its variables, a zero-initialised static array referenced only by a never-called inline
  reproduces the gap.
- A `goto RESTART` outer loop vs `for(;;)` changes where gcse hoists loop-invariant `lis` (main loop).
- Hand-rolled `goto` loops (label + `if (...) goto loop`) get no loop notes: no invariant hoisting
  and no givs, with an explicit `ofs += N` variable. A `found:` label inside the last loop's if-body
  puts the shared exit block inside that loop.
- A unit-owned global pointer the original reloads after every store through it is reproduced by
  defining the global itself as a one-member struct (`TexRenderMngPtr g_pMgr; g_pMgr.p`).
- Vec by-value parameters are passed by reference under the V4 ABI: callee code identical to `Vec*`.
- A local `lim = 512.0f` shared by an `if` test and a `while` bound keeps one constant register; a
  repeated literal inside the loop is hoisted as a second pseudo and copied (`fmr`).
- Manager work-scan loops whose range check survives only at the loop top come from a guarded
  do-while (`i = 0; if (i < n) do { p = getWork(i); ... } while (++i < n);`) with the inline `getWork`
  reading its fields through a local copy `cMgr* m = this;` (defeats thread_jumps). See map_obj.h.
- In-class inlines of a vtable-owning class are emitted in declaration order, but an explicit
  `virtual ~Mgr() {}` moves the base template's destructor relative to them; the implicit destructor
  gives the DOL order. Manager accessors like `getWork` should be free `static inline XxxMgrWork()`.
- `static int x = 0;` goes to `.sdata` with an explicit zero; unreferenced statics are still emitted.
  `_GLOBAL_.I.<key>` is keyed to the first *initialized* public object or function; `.bss` globals
  don't count.
- Switch tree rule (stmt.c): after merging consecutive same-target cases into ranges, with n nodes
  and r ranges the root is the node where cumulative cost (1 per node, 2 per range) reaches
  (n+r+1)/2; exactly 3 nodes -> middle. Default-equal `case X: break;` labels shape the tree, so
  enumerate every state the original enumerated.
- `if (a != 0 && a >= b) return 1; return 0;` keeps `cmplw; li r3,1; bgelr; li r3,0`;
  `return a != 0 && a >= b` gives the `subfc/adde` store flag. `ret = f(); if (ret != 0) {...}
  return ret;` yields `mr. r3,r3`.
- Inline accessors used as call arguments make precompute_arguments evaluate them before the
  stack-argument stores; plain member reads reload the pointer after each store.
- VLA (`T* tbl[n]`) gives `stwux` plus `mr r25,r1` / `mr r1,r25`; `alloca` has no restore.
- Empty `C() {}` / `~C() {}` produce the empty `__static_initialization_and_destruction_0` and both
  `global constructors/destructors keyed to` functions.
- Case bodies identical to `default` written as separate `case N:` arms survive as explicit
  `cmpwi N; beq default`; cases grouped with `default:` vanish but still count in the tree balance.
- A byte field passed straight to a call whose prototype takes `u8` yields `lbz; clrlwi; mr` copies;
  with an `int` prototype the load folds into the argument register.
- A run of literal zero stores plus one `= zeroVar` store: the `zeroVar` store is emitted first, the
  literals follow in source order.
- A `static const Vec` shared by two functions with one `.rodata` copy lives in an inline helper
  parsed before both.
- Register-argument addresses (`&local`) are precomputed into pseudos and cse merges later `&local`
  uses in the same extended block; only the frame-offset-0 local is set straight into the hard reg and
  recomputed. OPEN (sscrn FadeSet colour temps, `&pos` for setAng): how the original gets fresh
  `addi rX,r1,ofs` per call.
- KNOWN DEBT: `cUnit::beginEvent/endEvent` take an `int` in the original (sce_com loops, sscrn); the
  shared declaration in cManager.h is still `()`. Fix together with the em*/obj* owners.
- A `&local` passed directly to a call is copied to a pseudo and PRE hoists it into a callee-saved
  register (`mr rX,r4` ... `mr r4,rX`); the same call inside an inlined `static inline` helper taking
  `Vec*` gets the address as a hard-register arg set that gcse never sees, so it is recomputed
  `addi r4,r1,off` at each call. This is the lever for the OPEN `&local` reuse cases.
- `alpha * rate * helper(...)`: the (inlined) call is evaluated first, so `lfs alpha` lands after the
  `bl`; `helper(..., alpha * rate)` precomputes the product before the call.
- sched1 `adjust_priority`: an insn whose dependences resolve is boosted only if its dest register is
  live at block end and set once ("birthing"). A LATER CALL in the same basic block marks the FP arg
  registers live (call-clobber rule), so the `fmr` arg copies of a preceding `init(...)` call get
  boosted above its `li`s. That is why obj20's `cAtariInfo::init` (followed by `setPriority()` in the
  same block) matches while SetTrolley/SetGondola/SetYagura/SetHeliMissile/setScrAtari (no later call
  in the block) don't: the seven `Set*` targets have NO later call in the block (verified), so the boost comes from
  something else there — still OPEN. Note `birthing_insn_p` uses `bb_live_regs` left over from the
  weight scan, which only matters for single-block scheduling regions.
- `(f32)(int) w->u8field` gives the signed double trick instead of `psq_l qr2`; a `u32 x:8` bitfield
  gives the unsigned trick.
- Struct copy from a global pointer with a reload between (`obj->pos = pPL->pos; obj->rot = pPL->rot`
  -> `lwz pPL` twice) = `memcpy((u8*)obj + offsetof(pos), &pPL->pos, sizeof(Vec))`.
- `dx*dx + dy*dy + dz*dz`: the first product is fused into the second (`fmuls dy; fmadds dx`); the
  standalone `fmuls` is the second term.
- A local `class cEmRoom : public cEm` with N declared, undefined virtuals lets a unit call a
  room-module enemy virtual slot without emitting a vtable (key function undefined).
- Call arguments written as locals (`int parts = 0; f32 zero = 0.0f; init(parts, 2, parts, zero, ..)`)
  create pseudos cse keeps across later calls: the float lands in a callee-saved register reused for
  later `= 0.0f` stores, the int becomes the oldest zero pseudo every later `= 0` store reuses. Plain
  constants give hard-register chains (`lfs f1; fmr f2,f1`) and fresh loads later.
- sched1 flushes the pending memory list after 32 entries: in a block of >33 independent memory insns
  the 34th becomes a barrier; within each half stores are grouped by source register in RTL order.
- A float variable assigned in two places is never tied to a call's return register (`fmr f12,f1`
  right after the `bl`).
- Loop pointers must be block-scoped (`T* n = &node[i];` inside each body) to be replaceable givs; a
  function-scope pointer reused across loops leaves an `mr rN,rGIV` copy.
- Spilled `&local` pseudos get their stack slots in gcse hash order (table size = max_uid/4 | 1), so
  slot rotation between ours and the target means the original has more RTL somewhere in the function.
- A local at frame offset 0 is materialised per call; a block-scoped `Vec* pp = &p0;` declared after
  the first use gives "first use direct, later uses from a callee-saved pseudo".
- `p = Vec()` for a POD in g++ 2.95 creates a zeroed temporary plus a block copy, not a `memset`.
- `#line` must precede the *first* `__FILE__` use in the .cpp (dead functions included), otherwise a
  second, shorter file-name string appears in `.rodata`.
- PRE copy signature: `lfs f0; fadds ..,f0; fmr f11,f0` = a member load reused in later blocks by
  gcse (no local); a member cached in a local is used straight with no `fmr`.
- Index-first `lhzx/lfsx/add rD,idx,base` = `(T*)(i * sizeof(T) + (u32)base)` written index first.
- An early `return 0.0f` merged with the final return inserts a label that invalidates reload_cse:
  the following call re-copies a still-valid argument (`mr r4,r31`).
- `fabsf` as volatile asm is a scheduling barrier; where the target loads an `.sdata` constant before
  the `fabs`, use `__builtin_fabsf`.
- Function-scope temporaries reused across several tests become global pseudos that inherit hard-reg
  preferences; inlining the expressions per test gives per-block pseudos.
- Unused `.sdata` globals with no Bio4.sym name must keep the `lbl_XXXXXXXX` symbol name or
  strip_unused removes them.
- Reading a static through a reference (`static inline f32 FRef(f32& v) { return v; }`) gives a MEM
  with neither the struct nor the scalar flag: the load stays below preceding member stores and
  blocks flow.c's dead-store elimination of an earlier store to the same member.
- SN's `BRANCH_COST` is 0, so `&&` is never folded to `&`: a `subfic/adde ... and.` store-flag pair is
  an explicit `&` in the source.
- Byte stores of the literal `0xFF` share one `li rX,0xff`; a `u8`/`int` local `c = 0xFF` yields `li -1`.
- A constant shared by two functions but emitted between them is a public `const Vec x = {..}`
  (declared `extern const` first, defined at that point); `static const` at file scope is deferred.
- `if (x <= 0.0f)` gives `cror un,eq,lt; bso`; `if (!(x > 0.0f))` gives a plain `ble`.
- An address-taken local the original reloads after every store through it is a one-member struct
  local (`struct { cEsp* p; } e; PullEsp(&e.p, id)`).
- `static f32 v = 1.0f / (f32) n;` (function-local, runtime initialiser) is the `_.tmp_0` guard word.
- Brute-force harness note: ninja does not notice sub-second source rewrites; delete the .o before
  each rebuild or the scores are stale.
- Byte stores through `this` are output-dependent on word stores through a work pointer (different
  base pseudos), so they always follow them in a state-store block.
- `return t == 0` with `u32 t = x & MASK` gives `andis.; mfcr; extrwi`; `return (x & MASK) != 0` gives
  the bit-extract; `return t != 0` falls to the `li 1/bnelr/li 0` jump form.
- Search loop `for (s = tbl;; s++) { if (s->id == END) return 0; if (id == s->id) break; } return s;`
  gives the rotation with both tests at the bottom; a `return` inside the loop is not a jump to
  end_label. `continue` keeps the loop label used, so `addi; lhz` at the bottom are not combined
  into `lhzu`.
- Per-loop block-scoped `for (int i ...)` counters change pseudo numbers and hence gcse's hash order
  for PRE-hoisted increments and scratch numbering.
- A small object referenced with the full `lis/addi` form although it sits in `.sdata`: an
  incomplete `extern T x[];` declaration before the definition.
- Constant-pool order via `const f32 x = C;` locals at the function top: the initialiser is expanded
  at the declaration (constant enters the pool first) while every use is folded to the literal.
- Identical case bodies collapse into one only when the body that falls through into the join is one
  of them: `default:` must share the `case 0:` label, not be a separate trailing body.
- Loop invariants assigned inside the body (`range = to;` in the `for`) survive as an `fmr` copy /
  shared double-trick registers hoisted by loop.c; the bound `j < i` with `i = 15` a variable gives
  `cmpwi 0xf; blt`, a literal 15 gives `cmpwi 0xe; ble`.
- The original stores to unit globals/members through scalar references far more often than
  expected: `U16Set`, `VSet(ptr, MEM_ALLOC(..))`, `MSet`, `FSet(m->dir.y, -1.0f)` and reference
  *reads* (`BitChk(pG->flags, bit)`) are what keep following `.sdata`/`pG` loads below the store.
- Inline accessors taking `&m->member`: every such argument is a fresh `(plus m ofs)` that gcse PRE
  turns into an `mr rX,rMember` copy.
- `found = 1` written after a void call is scheduled above the `bl` into the callee-saved register.
- A `goto LABEL` loop keeps the un-rotated body/test/`b` shape; `for(;;)`+`break` gets rotated.
- Static locals show as `name.NNN` in objdiff; the DECL_UID suffix cannot be reproduced and is ignored by the report.
- Unexplained words in `.rodata` (zero words, stray floats) are usually the constant pool of a function
  the original linker dead-stripped (bodies gone, pools kept, `STRIP_UNUSED` in objects.py): write a
  never-called `static` function whose pool is exactly those constants at the right position (pools are
  emitted per function, right before its body; `x != 0.0` gives a DF zero, `x != 0.0f` an SF zero) and add
  the unit to `STRIP_UNUSED`. Aggregate templates/strings of `static inline` bodies are emitted at parse
  time, so a dead function parsed before a live one can own strings the live one also uses
  (`src/game/geometry.cpp`, `shape.cpp`).
- The compiler never emits `psq_l f,0(rX),1,qrN` straight from a pointer; that is inline asm in the original
  (`asm volatile("psq_l %0,0(%1),1,5" : "=f"(f) : "b"(p))`, store side `psq_st ... : "memory"`), with the
  pointer post-incremented in the operand (`PSQ_L_S16(src++)`) and a `s16 tmp[1]` function-scope array as
  the store target (`shape.cpp CalculateShape_new`). A compiler `(f32)` of a u16/s16 memory operand always
  goes `lhz; sth tmp; psq_l tmp`.
- `add rD, rA, rB` operand order: pointer arithmetic puts the pointer first (`ptr + i*8` → `add tbl, idx`),
  integer arithmetic keeps the written order (`i*8 + (u32) tbl` → `add idx, tbl`).
- `(u8*)d + (n * 2 + 3)` folds the constant into the multiply result first; `p = a + b; p = (p + 3) & ~3`
  keeps the sum in its own register (`add r10; addi r9, r10, 3; clrrwi r10, r9`).
- Parameter order of a function is only visible through prologue copy order (`mr`/`fmr` interleaving):
  `(..., f32 rate, u8* dst)` copies `f31` before `r29`.
- Cross-jumped return tails: `if (x) { if (cond) return 0; ... return fd; } return 0;` shares the outer
  `li r3,0` with the inner early return (`file_open`); `if (call()) return -1; else ret = 0;` inside the
  outer `if` keeps two `li r3,-1` (`file_close`).
- Screen filters (filter01/03/04/07/09/0b): `(u32) Screen.width` gives the `fcmpu 2^31/bso/xoris` unsigned
  conversion; `SCR_W >> 1` then `divwu` is `(SCR_W >> 1) / div`. The `GXSetCopyFilter..GXInvalidateTexAll`
  copy block keeps `&Rmode` in a callee-saved register only with a `GXRenderModeObj* rm = &Rmode` local.
  Static `u8 vfilter[7]` tables are `__attribute__((aligned(32)))` (the 0x1C `.sdata` holes) and each unit
  ends with `asm(".section .sdata; .balign 32")`.
- A variable reassigned through itself (`zv = f(zv)`) keeps its register; a fresh expression in the call
  argument combines with the dying operand (`fmadds f12,f12,...` vs `fmadds f13,...`). `a = x - 1.0f;
  a *= 25.0f;` ties `lfs/fsubs/fmuls` to one register; `a = (x - 1.0f) * 25.0f` gives three.
- `zero = 0.0f; call(); y = zero;` loads the constant before the call (the scheduler moves a pool load
  above a call); with the assignment after the call it stays after. A dead `f32 x = 0.0f` initialiser at
  the top only decides the constant pool order (the load itself is deleted).
- Float-in-u8 round trips: `(f32) a` of a `u8 a = 0x60` local in the same block folds to `96.0f`; the
  original keeps `stb/psq_l qr2` because the assignment is in another block (`a = 0x60` at the loop top,
  use after a `switch`) or the value is an `int` cast down: `(f32)(u8) a`.
- `u32 t = Joy[0].on & 0x400; f(t == 0)` gives `andi.; mfcr; extrwi ..,1,2` (store-flag); the inline
  `(x & 0x400) == 0` gives `xori/extrwi`. `GXColor amb = {0,0,0,0xFF}` declared right before its use
  (C++ mid-block declaration) keeps the `stw 0 / stb` next to the call; at the top it is hoisted.
  `u8 c = 0xFF; amb.r = amb.g = amb.b = amb.a = c;` gives the `li -1` QImode chain.
- A conditional `x12F = a < 250.0f ? 1 : 0` compiles to `mfcr`; the original `if/else` with constant
  stores cross-jumps to `li 0; bge; li 1; stb`. `f32 ratio = a / b; w->a = (f32) c * ratio;` evaluates
  the division before the u8 load; the inline product converts `c` first.
- Stores through `FSet` (scalar references) keep a following `.sdata` load (`lfs f1, static@sda21`) after
  them; plain member stores let ProDG hoist the load. Their emission order still follows the reverse-order
  rule, so permute the statements (`speed.x, speed.y, speed.z, pos.y` in source gives `x, y, pos.y, z`).
- Parameter order only shows in register allocation of the copies; for the filter `GXDraw` helpers the
  order `(f32 x, y, z, u, v, u8 r, g, b, a, f32 scale, int div, int fmt, ...)` reproduces the target.
- Struct field offsets come from the load/store displacements; write real structs, not casts.
- `rlwinm rX,rX,0,MB,ME` with wraparound = `x &= ~bit`; `ori` = `|= bit`.
- Branch shape follows the source: `if/else if` chains vs `switch` produce different compare orders
  (see `AreaCheckOnOff` in `src/game/cam_ctrl.cpp`: the original is a `switch`).
- Return `u8`/`s8` fields: `lbz` alone = unsigned, `lbz`+`extsb` = signed.
- Do not add casts to raw offsets to force codegen. Do not rename globally visible identifiers away
  from the names in `sym_map.tsv`.
- Data (`.rodata`/`.data`/`.bss`) in your unit must also match: define the globals the unit owns
  (see `[.data]`/`[.bss]` in `unit_info.py` output) with the right sizes and initial values.
- A `lwz rX,0(rY); stw rX,0(rY)` no-op pair on a flag word is `volatile u32 flag` with an inlined
  setter whose constant argument folds to `flag |= 0` (dvd `cDvdQueue::setStatus(0)`); a volatile flag
  also explains every reload of the word right after a store to it (`flag &= ~a; flag |= b` → two
  RMW pairs). The `li r9,1; andi.; bne; li r9,0; cmpwi r9,0` chains are an inline
  `int chk(u32 b) { return (flag & b) ? 1 : 0; }`; `if (flag & b) return 1; return 0;` gives the
  reversed `li 0; andi.; beq; li 1` chain (dvd, sofdec.h `isPlay`).
- A store of a register that "happens" to hold a loop counter or a compared value is CSE reusing a
  register known to be a constant on that path: after `if (depth == 0) {...}` the else-branch stores of
  `= 0` use the `depth` register, and `pFilehead[depth]` becomes `lwz 0(rBase)` (index folded to 0)
  (dvd readInit). Likewise `step = 0` right after `switch (step)` in `case 0:` stores the switch register.
- `switch` on a `u32` field gives `cmplwi` in the compare tree (dvd `DvdHeader::type`, `RomFontMessage`'s
  `u32 msg`), `int` gives `cmpwi`; a tree whose root is the lowest case with `ble default` comes from an
  extra empty `case` below it (`case ST_READ: break;` in `readCheckMain`).
- `if (a == 2 || a == 3 || ... )` on the same lvalue is range-folded (`subi; cmplwi`); five separate
  `if (x == k) return 1;` statements keep the compare chain (dvd `SysIsEurope`).
- A peeled first iteration (`lwz n = p->next; cmpwi; beq; cmpw n, q; bne loop` before the loop) with the
  hit block duplicated is `for (p = list; p->next; p = p->next) { if (p->next == q) { ...; ret = 1;
  break; } }` — `return 1` inside the body gives the rotated single-test loop instead (dvd DmaCancel).
- An `if (c) { ret = X; } else if (...)`  chain whose `ret = X` blocks sit *after* the main body means the
  source tested the inverse and put the big block first: `if (req >= 0) { ... } else ret = req;`.
- `cDvd* d = &Dvd; ... d->pList` and plain `Dvd.pList` are not equivalent for GCSE: writing the global
  directly gives the `mr r7,r9` address copy and a reload of `Dvd.pList` inside the loop that a local
  pointer lets the compiler hoist (dvd LinkQueue).
- Two copies of an address (`addi r0,r31,0xa8; mr r3,r0; mr r28,r0`) come from `sprintf(name, ...);
  n = name;` (statement after the call) — `n = name; sprintf(n, ...)` coalesces them.
- `for (i = 0; i < N; i++) a[i] = f(a[i])` over a constant-size array has no entry test and compares the
  pointer to the last element (`cmplw; ble`); `for (p = a; p <= &a[N-1]; p++)` keeps an entry test. A
  do-while over a global array with the base kept in a function-scope pointer (`DvdSndStr* pStr =
  Snd.str; s = pStr; do {...} while (++s <= &pStr[3]);`) hoists the `lis/addi` to the function top.
- `pos = mes_pos[lang][0]; pos += no * 2; f(pos[0], pos[1])` gives `lhzux`; indexing `pos[no*2]`,
  `pos[no*2+1]` gives `lhzx` + `add`. Local array initialisers are copied after the declarations that
  precede them in the source: a `pSys->x` read must be declared before the `u16 tbl[12] = {...}` to be
  loaded first.
- Callers passing an `int` to a `u8` parameter emit `clrlwi r4,r4,24`; if the target has it, the outer
  function's own parameter is `int`, not `u8` (dvd `ReadNblk2Blk(int)`, `ReadCancel(int, int)`).
- `bool ok = f(); if (ok)` materialises `li 1; bne; li 0; cmpwi` from the call result (dvd Watcher).
- `int v = 1; if ((pG->flags & bit) == 0) v = 0;` is how a stored/compared flag test is written when the
  target shows the `li/andis./bne/li` chain for a global; `(pG->flags & bit) ? 1 : 0` becomes `extrwi`.
- File-scope `static const` aggregates are deferred to the end of the unit (they land after the
  cManager template strings); a function-local static is emitted at its declaration, and a constant
  shared by two functions at the front of `.rodata` is a class static member (`const Vec cItemObj::zero`,
  obj19), which is emitted at its definition like any public object.
- Class vtables are `_vt.<len><Class>`; when another unit's split object references the vtable (an
  implicit inline constructor inlined into `cObjMgr::construct`) the symbols.txt entry must carry that
  name or ngcld exits 99 silently. `sync_symbols.py` renames `X_virtual_table` placeholders now, and
  `strip_unused.py` keeps them. A unit in `STRIP_UNUSED` must be synced once *without* the strip (the
  strip deletes every function whose name is not yet in sym_map).
- `switch` tree shapes: `case 0: case 1:` alone gives the linear `cmpwi 0; beq; cmpwi 1; beq; b default`;
  an extra empty `case 2: break;` gives the balanced `cmpwi 1; beq; bgt default; cmpwi 0; bne default`
  (obj26). Case labels that share the default body still shape the tree (`balance_case_nodes` counts
  every node, a range as two) even though jump threading collapses their compares into `b default`:
  obj14's weapon switch needs `case 5..0xA, 0xD, 0xF, 0x10, 0x12..0x18, 0x21, 0x28..0x2A, 0x2C, 0x2D:
  default:` to reproduce the root/branch compares.
- Independent constant stores at a block end come out in an order that is neither source nor reverse
  (`[a,b,c,d]` often as `d,a,b,c`, float and integer stores interleaved by load latency); when several
  fields are initialised, brute-force the statement order with a scripted variant loop (obj14ClothSet,
  SetObj08 took ~10 variants each). Constant registers: an `SImode` zero store followed by a `u8` zero
  store shares one `li 0`; the narrower store first gives two registers. Same for `-1`/`0xFFFF`.
- A struct copy into `pG->member`, `memcpy(&pG->m, ...)`, `(u8*)&pG->m`, `&pG->m.x` or a `Vec&`/`Vec*`
  helper all leave `pG` in its register for the next store; only `memcpy((u8*)pG + offset, ...)`
  (byte-pointer arithmetic destination: not `MEM_IN_STRUCT_P`, alias set 0) makes the next `pG->x = v`
  reload `pG` (obj14 `obj14_R1_Set`, esp1b). A `void*` destination is not inlined at all.
- `stage_no`/`room_no` are also read as one `u16` (`lhz 0x4f9c; cmpwi 4` = room 004): `GlobalWork::room_id`
  union. The four status bytes at cModel+0xFC are also compared as a word (`lwz; clrrwi 16; xoris 0x0100;
  subfic; adde` = `(stat & 0xFFFF0000) == 0x01000000`): `cModel::stat` union (obj14 ckBreak).
- `p ? p->id : 0` as a call argument gives two branches with `b`; `int id = 0; if (p) id = p->id;` gives
  `li 0; cmpwi; beq; lbz` (obj08 SndCall).
- Writing a sub-struct through its own pointer (`c = &w->cloth; c->x = ..`) makes CSE re-base every
  store on `&w->cloth`; `w->cloth.x = ..` keeps the work base with the larger displacements and a
  separate `addi` for the `&w->cloth` argument (obj14ClothSet).
- In a `for (i = 0; i < n; i++)` over an array of pairs, the member used several times (`list[i].part`)
  is strength-reduced into a pointer (`lwz 0(rP); addi rP,8`) while the one used once stays `lwzx`
  indexed off the array base (obj08ToEmHitCk).
- `MotionSetCore` has C++ linkage (`MotionSetCore__FP6cModelPvT1iiii`); `MotionMove`, `EmAtCheck`,
  `SetEmHit`, `YarareInit`, `EmGetDmPos`, `EmDmBloodSet2`, `PenClothSet/Move3`, `EstSet` are C.
- Header inlines that only *reference* a template member still instantiate it: the obj/esp/filter units
  carry the four `cManager<cLight>::create(int)` strings and the `create() failed %s id:%d` one of
  `create(int, u32)` because light.h declares `cLightMgr::createNew()`/`createNo()` (never called in
  the DOL; kept as out-of-class inlines so light.cpp does not get bodies).
- `if (c) n = a; else n = b + i * k;` where the else arm is a single simple set is emitted as
  `n = b + i*k; cmp; bne; n = a;` (jump.c hoists the else move above the branch); the sequential
  `n = b + i*k; if (c) n = a;` gives an `mr A',A` copy of the array base instead (obj00 FallMove).
- Local array element access `s[i].f.x` folds the field offset into the frame base (`frame+0x48 +
  i*stride`, indexed stores); `p = &s[i]; p->f.x` gives the stepping pointer with displacements
  (`stfs 0x18(r7); addi r7,0x2c`). A block-scoped `p` becomes "not always computable" when it is set
  after a conditional inside the loop, which stops biv elimination (`cmpwi i,2` stays instead of the
  pointer compare): set `p = &s[i]` before the `if`.
- `case 7: break;`-style nodes that share the default body still shape the tree (obj18 move needs
  `case 0xB: break;`), and a two-value range `case 2: case 3:` emits `cmplwi hi; bgt; cmplwi lo; blt`
  where `if (x >= 2 && x <= 3)` is range-folded to `subi/cmplwi` (obj01AddSpeed).
- `w->x &= ~2; w->x &= ~0x20;` gives two `rlwinm` on one load/store (combine refuses the non-mask
  constant); `&= ~0x22` loads the constant into a register (obj1d Lost).
- `(u16) w->word` from memory is loaded as `lhz +2` (combine narrows the load); a `(f32)` of an s16
  array element is `lhz; sth; psq_l qr5` (qr5 = s16, qr3 = u16).
- Parts matrix normalisation: `if (v.x == 0 && v.y == 0 && v.z == 0) v.x = 1.0f;` before each
  `VECNormalize` (obj00/obj1d SetOya), the column loads/stores are plain `m[r][c]` scalars.
- An opaque manager class must carry its real size (`u8 pad[0x34]`) or `extern cDmgMgr DmgMgr`
  lands in small data (`li r3, DmgMgr@sda21`).
- NgcAs emits `R_PPC_REL14` relocations for `bc` (conditional) branches to local labels; objdiff
  then shows the loop body as REPLACE lines although the bytes are identical (template copy loops).
- OPEN: independent `li rX,c` argument loads of a call are sometimes scheduled in a different order
  than ours (obj00 `setScrAtari` interleaves int/float arg moves, obj01 `EstSet` calls put `li r3,0`
  first while ours emits it last); the ProDG register-pressure tie-break is not understood.
- (camera units) `(&cam->member)->y` stays pointer arithmetic (GCC 2.x builds `&x->m` as base+offset), so
  matrix/column helpers taking `Vec*` (`getColumn(m, c, &v)`, `setColumns(m, &a, &b, &c, &d)` as static
  inline functions in cam_sys) give `stfs 4(rP)` through the address register with the `.x` store via the
  frame/base (cse picks the zero-offset form); plain `v.y = m[1][c]` gives frame-relative stores.
- PRE (gcse) creates a pseudo set in *both* branches (`mr r27, r5` after the existing `addi`) for an
  inline-parameter address computed in each arm and used after the join; call-argument `addi rN,r1,off`
  are hard-register sets and are never PRE'd (a fresh `addi r4, r1, 8` after the join).
- `w->maxFrame = (f32)(k & 0x3FFF); w->maxFrame += 1.0f;` (two statements) ties the add to the 1.0
  register with the double trick first in the pool; `(f32)x + 1.0f` ties to the converted value.
- `tbl = (u32*)((u32)w->partsNo + w->nParts); tbl = (u32*)(((u32)tbl + 3) & ~3);` keeps the sum in its
  own register (cam_motion ctor) where the single expression reuses it.
- Mtx copy loops (`while (i_--) { for (j...) *dp_++ = *sp_++; }`, MTX_COPY in motion.cpp/camera.cpp):
  declaring/incrementing `d_` before `s_` decides which pointer gets r9/r11 and the `addi` order.
- `for (i = 0; i < 2; i++) memclr_asm(&g_Arr[i], n)` gives a pointer loop with a *signed* `cmpw` end test;
  the do/while pointer form gives `cmplw`. `for (j = 0; j < 1; j++) a[j] = 0` with `u32 j` gives the odd
  `li r0,0; sth; addic. r0,r0,1; beq` one-iteration loop (ctrl12 move).
- A `pLog->warn()` path that falls off the end of a non-void function (`if (!p) { warn; } else { ...
  return idx; }`) leaves r3 = the warn call's r3 (trans_ot AddOt*Radius).
- vtable emission order at finish_file is reverse class-declaration order, and marking a vtable that
  holds a not-yet-instantiated template member (cManager<T>::destroy) instantiates it right there, so its
  strings land between vtable groups: ctrl.h declares cCtrl00/01/10 *after* cCtrlMgr to get
  `_vt.7cCtrl10, 01, 00, [destroy strings], _vt.8cCtrlMgr, cManager, cCtrl, cUnit`.
- An in-class inline `getWork()`/ctor of a class whose vtable the unit owns is emitted out of line (grows
  .text): use a free `static inline` (ctrl.h `CtrlMgrWork`) and no user-declared `cCtrl()` ctor.
- Functions with function-pointer parameters declared inside `extern "C" {}` need `extern "C"` on the
  definition too, or GCC 2.95 treats the definition as a C++ overload (`AddOtDirect__FiPvPFv_v...`).
- Weak vtable copies (`_vt.7cCamera` in cam_extra and cam_motion): the reference in the later unit binds
  to the first copy program-wide, so the first unit's copy must carry the `_vt.` name in symbols.txt and
  the later copy a distinct one (`cCamera_virtual_table_8022C140`), otherwise the DOL differs.
- `#line N` for an inline `MEM_ALLOC` inside a class body counts from the `class` line (ctrl.h: `#line 113`
  puts memAlloc on line 116).
- `-0x602` mask (`and r0, r0, r11`) in every `_._7cCtrlXX` is the inlined `cUnit::~cUnit` (`be_flag &= ~0x601`).
- (read) `bl ReadCheck__4cDvdi` with a `DvdReadInfo*` in r5: `cDvd::ReadCheck(int)` passes its uninitialised
  `info` pointer through to readCheckMain, and read.cpp calls it with three arguments. An asm-labelled
  member declaration reproduces the call (`int ReadCheckInfo(int, DvdReadInfo*) asm("ReadCheck__4cDvdi");`
  in dvd.h); PMF casts give an indirect call.
- `static` functions with unmangled names in sym_map (decodeData, readEm) were declared inside the
  `extern "C" {}` block. "global constructors keyed to X": X is the first *public* function/initialised
  object emitted, so everything before it in `.text` is static.
- `memcpy(dst, src, n)` with typed pointers (`OSModuleHeader*`) expands inline to a libcall with
  `crclr cr1eq`; `void*` operands give a plain prototyped `bl memcpy` (read readEmData).
- A store to a scalar global followed by a struct-member load (`EmInitFunc = m->pInitFunc; return
  m->pArc;`) lets ProDG hoist the load above the store; the original kept the order, so the global is
  stored through a struct view (`((EmInitFuncPtr*)&EmInitFunc)->p`, the pLog trick on the store side).
- A flag test through a `u16&` inline (`BitChk16(PlReadModule.flag, 2)`) materialises `sym+0x82` into a
  register; `&PlReadModule` right after is then `subi r4, rX, 0x82` (cse related-value). `BitOn16` on a
  global struct member gives `lis sym+ofs@ha; lhz/sth sym+ofs@l(r9)`, the plain `|=` after `&sym` is
  known gives `addi sym@l` + displacement.
- `lbzu r0, 4(rP)` at a loop top = `p += 4;` as the first statement of the body (combine merges the add
  into the load); the biv init `p = (u8*)arc + n*4; p += 0xC;` as two statements gives `add rP; addi rP,rP`
  where one expression leaves a temp (`add r9; addi rP, r9`).
- Independent struct stores at a function end: the *last* source statement is issued first, then the rest
  in order (`id, pArc, size, pModule, bssSize` -> `bssSize, id, pArc, size, pModule`).
- `if (f() == 0) { fail; return 0; } success; return x;` lays the success block out as the fallthrough
  (`beq fail`); the if/else form puts the fail block first.
- A constant stored to a global and then assigned to a local (`pG->pPlArc = C; data = (u8*)C;`) is
  cse'd into a copy and re-materialised as `lis/ori` before the preceding call; a local initialised at
  the top shares its register with the store (ReadPlayerData).
- `__attribute__((aligned(32)))` objects of size 0x98 in `.bss` (ReadModule) leave the 8-byte holes;
  the unit ends with `asm(".section .bss; .balign 32")`.
- OPEN (read ReadPlayerData): the PRE'd `type == 0` compare lives in a GPR (`mfcr r29`/`mtcrf 128`) in the
  original, in cr4 in ours; the pass-0 "already used register" choice depends on the GPR allocation order
  (r29 = pArc in the original). readEmData `m`/`newSize` swap r28/r29 (m has 14 refs, one more flips it).
- newlib units: SN's shipped `va-ppc.h` (v393/include) differs from gcc's: `char gpr/fpr` (signed), its
  own `va_arg` (`gpr + size <= 8`, `__va_longlong_p`) — `include/va_ppc.h` carries it. `strtod` (game/strtod2)
  is the Tcl strtod with `float` mantissa/`float powersOf10[]`, `isspace()/isdigit()` unprototyped
  (`crclr`), `if (!isdigit(UCHAR(*p))) { p = pExp; goto done; }` after the exponent sign. vfscanf is the
  stock 1.8.2 source with `MB_CAPABLE` (`__mb_cur_max`, `_mbtowc_r`) and `u_char *__sccl ();` unprototyped.
- (trans_ot/room_jmp/sce_sys) `-fcse-skip-blocks` rewrites a load after a skipped `if` block with the
  address `sym+off(rBase)` it knew before the branch, keeping the base symbol live across a call
  (`lwz r9, 0x88(r24)` + a second callee-saved register). The original avoided it with a different CFG
  in front: `if (zlimit == 0.0f) z = 0.0f; else {...}` instead of `z = 0.0f; if (zlimit != 0.0f) {...}`
  (AddOtWorldPos). Test with `-fno-cse-skip-blocks` on `cc1plus` to confirm the pass.
- `&g_Table[CONST]` folds into `lis/la sym+off`; a `static inline T* tbl(int i) { return &g_Table[i]; }`
  accessor gives the original `addi rX, rSym, off` (trans_ot `otWork(17)`, room_jmp `ofsTbl(p)[i]`,
  sce_sys `eventFlags()[no >> 5]` for `lwzx/stwx` on a pG-relative array).
- Argument order in the C prototype is fixed by the incoming-copy order at function entry: ints and
  floats are assigned registers independently, so `(Vec* pos, f32 radius, u16 kind, f32 zlimit)` and
  `(..., u16 kind, f32 radius, ...)` produce the same call ABI but different `mr`/`fmr` order and
  callee-saved allocation (AddOtWorldPosRadius: kind r27 > func r26 only with radius declared before kind).
- A struct local whose target frame slot is 8 bytes bigger than its declared size means the struct is
  bigger in the original (GeoSphere is 0x18: trans_ot frames), not a compiler temp.
- `for (...) { if (hit) { call(); ...; break; } }` with a *call* in the hit path is not rotated (initial
  `b test`, test at the top); the rotated original (entry test duplicated, `bdnz`-less bottom test) comes
  from `i = 0; if (i < n) { do { ... break; ... i++; } while (i < n); }` (roomdata clear). Without the
  call the plain `for` rotates (roomdata load).
- A search loop whose result is used *after* the loop with the address recomputed inside the loop
  (`mulli; lis/addi Task; add; lbz`) had no loop notes: written with `goto` (sce_sys SceExec slot search).
- `p = start; while ((p = get(p)) != 0) { if (p->x == t) { ...; break; } }` hoists the tail's constants
  (`li r30,0`, `lis sym`) into callee-saved registers above the loop; `return` instead of `break` or a
  `do {} while (p->x != t)` gives the non-hoisted form (sce_sys SceTaskDelete needs `break`).
- A `u16` member written then immediately read back (`x1C = tbl->rel; if (x1C == 0) ...; f(x1C)`) is
  forwarded as `clrlwi rX, rStore, 16` in the original; ours folds it to `mr` (roomdata linkRelData, open).
- `static test test; ... tbl[test.state](&test)` re-forms the address each use; the original kept
  `&test` in r31: `struct test* w = &test;` and use `w` (room_jmp RoomJump).
- `int no; no = w->mode; switch (no) {...}` with the *same* int variable reused for a call result inside a
  case (`no = pRj->checkRoomNo(...); if (no >= 0)`) gives `mr. r30, r3` into the switch register;
  cse's jump equivalence stores that register where a `0` constant is needed in `case 0:` (room_jmp).
- Zero-initialised statics with an explicit `= 0` (`static f32 rdir = 0.0f`) go to `.sdata`, not `.sbss`;
  unreferenced initialised static locals are still emitted (cam_extra: `rnd_gain`, `rnd_on`, `sct_max`).
- Declaring a C callee as returning `int` instead of `void` (cam_extra `IdTexDataLoad`) makes its own
  `mr r3,rX` argument copy precede its `li` argument loads; the preceding call's return type
  (`IdTexRelease`, `void`) did not matter there.
- Passing a struct (not its member) to a varargs `%s` (`pLog->err(..., FileTbl[x1C])`) copies the 8 bytes
  to the stack and passes its address (roomdata linkRelData — a bug in the original kept as is).
- A class with a constructor and an *empty* `~T() {}` is what makes GCC 2.95 emit the
  `global destructors keyed to` function next to the constructor one (roomdata cRoomData); the key is the
  first emitted object, static or not (`St0_data_tbl` is non-static in the original).
- (sscrn) A register argument whose address is a plain `(plus fp N)` / `(addressof reg)` is precomputed
  into a pseudo (`preserve_subexpressions_p()` is 1 at -O2) and cse then merges every later `&local` of the
  same variable in the extended block into that pseudo (callee-saved `mr r4, rN` copies). Only the local at
  frame offset 0 (`(reg vsv)` at expand time) is set straight into the hard register and recomputed per
  call. The bare `&far`/`&ab` (`addi r5, r1, 8` twice) vs `mr r5, r30` (`&ac`) split in sub2 comes from this.
  OPEN: sscrn's fade colours (`stw 0xFF/0` into 8/0xC, `addi r4, r1, 8; addi r5, r1, 0xc` recomputed in
  OpeSetOpenTermEnd, slots shared with a block-scoped `ItemInfo`) and OpeSetOpenTerm's second `&pos`
  (fresh `addi r9, r1, 0x68` after `setPos(&pos)`): u32/union/GXColor/4-byte-class locals are spilled to
  fixed slots (no sibling reuse, `purge_addressof`), BLKmode/dtor-class temps share slots but cse merges
  the address; inline-with-parameters (`fadeW(no, u32 c0, u32 c1, t)`) gives fresh addresses but fixed
  slots. ~25 forms tried.
- `switch (lang) { case 1: ... case 2: ... }` with *identical* bodies keeps two tree nodes (bodies
  cross-jumped afterwards): `cmpwi 1; beq; bgt` then `cmpwi 0`; a shared `case 1: case 2:` label makes a
  range node and a different tree (sscrn sscrnSetLanguage).
- `switch (room) { case 0x111..0x113: case 0x118..0x11B: return room - 0x10; } return room;` (u16 in/out)
  gives the `cmpwi/bltlr/ble/bgtlr/bltlr` chain with `subi; clrlwi 16` (sscrn sscrnRoomNo).
- `(u8)(u32 & 0x10000000)` folds to 0 at the tree level; storing through a `u32 t = x & mask; w->b = t;`
  temporary keeps `rlwinm; stb` (sscrn x1B6, a harmless bug in the original).
- `if (size == 0) bss = 0; else bss = alloc(size);` puts the `li r4, 0` between the compare and the
  branch (jump.c moves the *first* arm's simple set above the jump); `void* bss = 0; if (size) ...`
  schedules the `li` before the loads (sscrn DLL_Link).
- `u32* tbl = pG->bits; BitOn(tbl[no >> 5], 0x80000000 >> (no & 0x1F))` (u32 `no`) gives
  `rlwinm 29,3,29` + `lwzx/stwx` off a materialised `pG + 0x82F0`; indexing `pG->bits[...]` directly folds
  the offset into the displacement (sscrn OpeSetMdtNo).
- Two struct-member zero stores in an `if` body come out reversed (`x2AF = 0; x2AE = 0` → `stb 2AE; stb 2AF`);
  three separate `= 0` statements `x; y; z` → `z, x, y` (sscrn GameInit / RoomInit / Miss).
- `Cckpt.getCountDown()->f()` (inline accessor returning `&member`) inside a `for(;;)` task loop is hoisted as
  a loop invariant (`addi r15, r9, Cckpt@l` at the top, `addi r3, r15, 0xb0` at the use); a block-local
  `Cockpit* ck = &Cckpt; ck->countDown.f()` is not (`addi r3, r27, Cckpt@l; addi r3, r3, 0xb0` at the use).
  SubScreenExec uses the first form, SubScreenExit the second.
- The original `cUnit::beginEvent`/`endEvent` take an `int` (sce_com's `cManager<T>::beginEvent(int)` loops
  pass r4, sscrn passes 0); the shared declaration is still `beginEvent()`, so sscrn calls it through a
  vtable-compatible view class (`BEGIN_EVENT`). Changing cUnit means touching every override (obj*/em*).
- A loop-local `u32 addr = *cs++` instead of reusing the function-level `pc` swaps the r28/r31 allocation of
  the two (exception ErrorHandler call-stack loop).
- (cockpit) MEM_IN_STRUCT_P decides which loads survive a store: `*(f32*)((u8*)v + i*4)` (cast then deref, no
  PLUS at the top of the INDIRECT_REF) is not in-struct, so a `static f32 a_ratio` is reloaded after every such
  store while the address still folds to base+index (`lfsx r9,r10`); `v[i]` / `*(v + i)` are in-struct (the
  load is hoisted) and an `f32&` parameter makes the address a general giv (stepping pointer with
  displacements). `FSet(unitPtr()->rot.z, x)` on a call result keeps the following `pG` load below the store.
- A `u8` function result (`u8 f()` declared so) assigned to a `u8` local is never masked (SUBREG_PROMOTED); an
  `int` local holding a u8 result passed to two u8 parameters gets one PRE'd `clrlwi` before both calls
  (BulletInfo::move `wepNo`). `u16 num = f()` with `u16 f()` compares `mr. r30,r3` directly and masks
  `clrlwi 16` only after `num /= 10`.
- A 2-byte `struct { u8 hi, lo; }` local lives in a GPR: `d.hi = v/10; d.lo = v%10` builds it with
  `clrlwi/slwi/or`, a later `d.lo = x` inserts with `rlwinm 0,16,23 | clrlwi 24`, `d.hi` reads as `srwi 8`
  (or `extrwi 8,16` when the hi insert was unmasked). One such variable reused for min/sec/cs shares r28;
  an inline returning the struct by value spills it to the frame.
- fold merges `!(x & A) && !(x & B)` on the same lvalue into one `andis.`; an inline `chk(u32 b) { return
  pG->f & b; }` per test keeps the two `andis.`/`bne` (CountDown::move).
- Extra `psq_l` pool copies in `.rodata` after the last float function = a dead-stripped function with the
  same conversion formula (cockpit `TIME_FRAME` third copy, `STRIP_UNUSED`).
- A `static` local `u8 cnt` / `char xchr[5]` pair (`cnt.NNNN` .sbss, `xchr.NNNN` .sdata) with a `%c` print is
  the spinner `xchr[cnt]; cnt = (cnt + 1) & 3` (debug processBarDisp); config-file flag toggles in
  ConfigSet are BitOn/BitOff (each `|=` reloads `pG`).
- A game unit followed by an SDK library unit carries absolute-address padding the assembler cannot
  reproduce with `.balign`: sscrn ends with `asm(".text\n\t.long 0, 0, 0")` and a never-referenced
  `static u8 pad[0x1C]` (kept alive by an unused inline) for the 0x1C `.bss` gap.
- (datactrl) Array scans over a member array: `for (int i...) u = &unit[i]` gives the pointer loop with a
  *signed* `cmpw` end test, `u32 i` gives `cmplw`; both without an entry test. A pointer loop
  (`for (u = unit; u <= &unit[31]; u++)`) adds an entry test.
- Switch tree shape, exactly: after `group_case_nodes` merges adjacent cases with the same target into
  ranges, `balance_case_nodes` splits a list of n nodes/r ranges at the node where a countdown from
  `(n + r + 1) / 2` (2 per range, 1 per single) reaches 0; lists of 3 split at the middle, lists of 1-2
  stay linear. Empty `case k: break;` labels shape the tree (they are nodes whose target is the default),
  so a root of 4 for values 1..8 means `case 0: break;` exists too; `case 5: case 6: case 7: case 8: break;`
  (a range) moves the root of {0..4} from 2 to 3 (setLoadToMram/Aram).
- A `case` body that falls through into the next case (`case 7: add(); /* fallthrough */ case 4: case 5:
  add(); break;`) is the source of "duplicated tail" case bodies whose second half is not CSE'd with the
  first (no reload merging across the label) (getAramFree).
- loop.c (`find_and_verify_loops`) moves a block that ends in a jump out of the loop (`return X` inside a
  loop, guarded by one conditional jump) to right after the nearest BARRIER outside all loops before the
  return label. `if (n == 0) return 0;` before the loop creates such a barrier and the block lands there;
  `if (n != 0) { loops } return 0;` leaves only the function-top early return, and the found block lands
  after `if (aramSort == 0) return 0;` (checkAramSort). Blocks containing an inner loop are not moved.
- A `default: return 1;` that shares the trailing `li r3,1` with the normal `return 1` after a store
  block: write `default: goto ret;` with `ret: return 1;` after the stores. Without the label sched1
  hoists the `li r3,1` above the stores and jump2 cannot cross-jump the default into it (setClear).
- HALT-style macros as a plain `{ ... }` block instead of `do { } while (0)`: the OSReport stays in the
  same basic block as the preceding `pLog->err`, so its `li r4,0` is issued before the string `addi r6`
  (anti-dependence on the later `lis r4`); the do-while form gives `lis/addi r6` first (setData).
- A constant shared by stores in the loop and after it from one callee-saved register
  (`sth r21, 0xc(rP)` = 0x1F8 in three blocks) is a local assigned once *after* the preceding call
  (`eprintf(...); x = 0x1F8;`); assigned at the function top it is scheduled before the call.
- `Debug_free_h`/`Mem_free_h` (main_mem) are global (datactrl calls them) although Bio4.sym marks
  them local; datactrl's `cDataUnit` empty ctor + `~cDataUnit() {}` reproduce the 32-element ctor/dtor
  loops of `DC`'s static initializer.
- OPEN (datactrl dispDebug): global-alloc order p(r31) > u(r30) > this(r29) in the original; ours gives
  the loop pointer giv r31, this r30, p r29 (same refs and instructions; loop forms, declaration order,
  `unit[i]` indexing, do/while tried).
- Index registers vs base registers (regclass `record_address_regs`): in `lwzx rD, rBase, rOfs` /
  `add rP, rBase, rOfs` the offset pseudo prefers GENERAL_REGS (r0 first in the alloc order) when the
  base pseudo is pointer-flagged (a pointer parameter/local), and BASE_REGS (r9/r11/r10...) when it is
  not — both operands then count half as base. A table offset the target keeps in r9 while ours takes
  r0 means the base was an integer (`u32 addr` parameter, `(EffData*) addr` local), not a pointer
  (eff_sys EspDataLoad).
- `y = C; loop { if (hit) { f(y); y += step; } }`: the target's `li rY, C` issued *last* in the preheader
  (after loop.c's hoisted `lis`/`addi`s) and living in the highest callee-saved register is a
  strength-reduced giv: `n = 0; ... f(C + n * step); n++` (esp EspMove, esp_app EffAreaUpdate).
- update_equiv_regs (local-alloc.c): a pseudo set once to a constant and used once *in another basic
  block* has its `li` moved to right before the use (short range, a caller-saved register such as r9
  right before the `stw`); the same constant written at the store, or a local declared in the same
  block, is hoisted by sched1 into a long-lived callee-saved register. `int repType = 1;` at the
  function top with `mgr->repType = repType;` inside the `if` (esp_app EffEm2d_setTexRender).
- A pointer local assigned in two places (`mgr = pMgr;` twice) has two deaths, is skipped by local-alloc
  and gets a caller-saved register (r12) from global alloc; two distinct locals are local-allocated and
  take r9 / r30 in alloc order.
- Reading a global through a one-member struct *wrapper declaration* (`TexRenderMngPtr g_pMgr; g_pMgr.p`)
  forces the `@sda21` address into a register (`li r8, g@sda21; lwz r9, 0(r8)`) whenever the member is
  read as a value (`mgr = g_pMgr.p`, `this` of a method call) — `expand_expr` calls `memory_address`
  on the BLKmode struct and constant addresses go through a pseudo "to be cse'd". A plain pointer
  global with reference-setter stores (`BitSet(mgr->sx, 0x40)`, `ISet(mgr->repType, v)`) gives the
  same reloads with direct `lwz r9, g@sda21`.
- jump.c hoists a first-arm single set (`if (c) on = 0; else { on = 1; ... }` → `li 0` before the
  branch, inverted test) only when the else arm *starts* with a set of the same variable; reading a
  global into a local first (`u32 f = pG->flags_5010; on = 1; if (f & bit) ...`) keeps the target's
  `beq; li r11,0; b` block (espgen EspgenIsActive).
- local-alloc ties a dying operand to the result of a 3-operand insn (`fmuls fD, fD, fC` with the
  `(f32)d` operand in fD) only when the result pseudo is block-local with one death; a float temp
  assigned in both arms of an `if` (`t = (f32)d * c; ret = 1 - rate * t; ... else t = ...`) is global,
  so nothing ties and d/c/1.0 take f13/f12/f11 after the products in f0 (espgen00/02 Calc_D256).
- cse_around_loop (cse.c): for a `for`/`while` loop whose latch ebb ends at LOOP_END jumping back to the
  header, an expression computed in the header that a REG_LOOP_TEST_P register of the latch also holds
  (`sys + 0x10000` for high-offset members) is rewritten as a copy of that register, with a second copy
  emitted after the matching computation before the loop: `mr r9,r10` before the loop and in the latch,
  the header load using r9 (esp `operator new` loop 3). Goto loops never get it.
- OPEN (esp `operator new`): writing that loop as `for` reproduces the copies but loop.c then moves the
  `PushEsp(esp); goto found;` block behind `found:` (the guarded-exit-block motion above); no form found
  that keeps both. Loops 1/2/4 must stay goto loops (a `for` strength-reduces `&esp->flag`).
- OPEN (esp_app EffAreaUpdate / esp45 HideCheck): a loop.c-hoisted `lis` and an independent `or`/`li`
  in the preheader are issued in the other order; both have equal priority/weight so the tie-break is
  the RTL (LUID) order of the hoisted insns, which no source form changed.
- OPEN (espgen02 espgen02_Update): three `f32 x = 0.0f` locals share one pool load; cse loads it into
  the variable whose last mention is latest in the insn chain (ours colR, target spdR) — the target
  mentions spdR after colR's `colA *= colR` somewhere we do not reproduce.
- OPEN (Espgen43 AddSandPower): the `lis/addi Chk_pos` pair and the z-word temp of the 12-byte struct
  copy swap r10/r11 (local-alloc priority order); memberwise, memcpy, pointer and statement-order forms
  tried. SetSandWork also has an unidentified 8-byte frame slot and one more callee-saved GPR.

## Don'ts

- Never change the semantics of a shared tool (strip_unused.py, fold_linkonce.py, sync_symbols.py,
  ngccc.py) to fit one unit. Make the new behaviour conditional on the evidence that distinguishes
  your case, and re-run the full `ninja` + DOL check before and after.

- Never run `git stash`, `git checkout -- <file>`, `git reset` or anything else that rewrites the shared
  working tree: other agents are editing it at the same time.

- Never edit `build/`, `build.ninja`, `objdiff.json`, or `config/G4BE08/splits.txt` by hand.
- Do not run interactive `objdiff-cli diff`; use `tools/fdiff.py` (one-shot).
- Do not commit; the orchestrator commits.

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
- The 110 REL overlays (rooms, enemies, players, weapons, tools) build byte-identical too; their units are `<mod>/<file>.cpp` with
  their own symbol files under `config/G4BE08/modules/<mod>/` — see "REL modules" below.
- Types: `include/types.h` (u8..f64). Shared class/struct definitions go in `include/<name>.h`;
  check existing headers before adding a type, and only extend, never rewrite, structs other units use.
- Globals seen via `r13`/`r2` (`@sda21`) are small-data; declare them `extern` with the exact symbol name.
- HAZARD: a module source that declares a DOL C-linkage function WITHOUT `extern "C"` (or a DOL
  static it imports) makes `sync_rel_symbols.py` rename the DOL symbol to the mangled/imported name
  in config/G4BE08/symbols.txt, breaking every other module's import (pl0e `Em_R0_Scenario` today).
  Declare DOL imports with the linkage the DOL header uses; after any module sync check
  `git diff config/G4BE08/symbols.txt` for unexpected DOL renames.

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


### Known compiler-build differences (v1.79 source vs the original build)

Two argument-passing behaviours of the original binary are not produced by any source form with our
cc1plus and are therefore compiler-build differences (the original is a later SN build):
1. Argument-move order at calls with mixed int/float args: the original sometimes issues FP arg moves
   before trailing integer constants (`mr r3; fmr f1; fmr f6; li r4; fmr f7; li r5; li r6` for
   `init(int,int,int,f32 x7)`), GCC 2.95's `load_register_parameters` never does. A whole-tree
   experiment harness exists at /home/adityas/Projects/re4-orig/sn-gcc-argorder/harness/ (10 s per
   run over all game units, with `calls.tsv` = 1209 classified call sites); every candidate rule
   either regresses matched functions or fails SetEmBarred's interleave, so NOTHING is installed.
   Workaround: a floats-first asm-labelled redeclaration (include/atari_init.h) for the affected callee.
2. Narrow-argument extension: the original sign/zero-extends narrow values at some call sites and
   entries (`extsh`, `clrlwi 24/16`) where ours treats them as promoted. Workaround: asm-labelled
   alias with the signed/narrow type (id_sys.h `setTimeS`).
3. (candidate) Frame-address PRE: no `addi rX,r1,N; mr rY,rX` pattern exists anywhere in the original
   asm, while our gcse routinely creates a pseudo for `&local` used in several blocks; the original
   also never cross-jumps a single-insn tail (`find_cross_jump` minimum). Under investigation with the
   harness; until then use the `&local` levers (frame-offset-0 local, inline helper taking `Vec*`).
4. Narrow-argument truncation: the original build does not truncate `int` -> `u16` arguments at call
   sites nor a wider value on a narrow `return`, but masks a u8-returning call assigned to a u16.
   Workaround: asm-labelled int-view / narrow-view declarations (item.h `constructI`, `searchI`).
5. haifa interblock scheduling: our cc1plus's `find_rgns` never marks leaf blocks (only successor =
   EXIT) as reached, so any function ending in a plain return block gets single-block regions and no
   interblock motion; the original formed regions there (shadow `ShadowTrans` `mr r3,r31` hoist over
   a small loop). A leaf-fixed build (/tmp/sngcc-leaf) forms the regions but then moves far more than
   the original, so the original's motion policy differs too. Compiler-build difference, not source.
6. Cross-jump survivor choice: our jump2 always keeps the *last* identical `li r3,1; b end` copy;
   the original sometimes keeps an earlier arm's copy and cross-jumps later ones into it (pl_class
   `isKamae`), and never merges single-insn tails (item `use`). Compiler-build difference.
Do not spend unit time on any of these; use the workarounds and move on. POLICY: every workaround
for a compiler-build difference (asm-labelled aliases, `asm("" : "+r"(x))` launders, `register ...
asm("rN")`, dead `p = 0` initialisers used only to shift gcse/loop.c counts) must carry a comment
`// COMPILER-DIFF: <which item>` so they can be removed mechanically if the original build turns up.

### Host-dependence of our cc1plus (negative result, do not re-investigate)
The configured cc1plus is a 32-bit static i386 build (HOST_WIDE_INT = int, like Win32). cse.c hashes
SYMBOL_REF/LABEL_REF by host pointer and varasm's const hashes include pointers, but they only select
a bucket; every lookup is exact-match and every ordering decision comes from insertion/call order.
All 610 ProDG units produce byte-identical `.s` under: heap base shifts, heap padding, ASLR on/off,
MALLOC_PERTURB_, pattern/zero stack fills, x87 53-bit precision (Win32 CW), an unstable heapsort as
qsort, cse/varasm/type hashes collapsed to one bucket, an -O0 host build and a clang host build (the
clang build differs only by an extra `.size` from an uninitialised local in SN's toplev.c patch, which
cannot change DOL/REL bytes). The residues described as "which equivalent register", "`lis sym@ha`
pseudo choice", "qty tie", "LUID tie" are v1.79-source-vs-original differences, not host effects.

### Later SN build search (negative, 2026-09-09)
No SN GCC for GameCube newer than cc1plus v1.76 (binary) / v1.79 (GPL source, 2003-06) exists in any
public place (decomp.me/decomp.dev packs, archive.org sn_sys_consoles_2 / prodg-gamecube / GameCubeSDK,
MarioCube, GitHub, Wayback of snsys.com whose support downloads were login-only). RE4's crt0 links
libsn v60 (public pack: v59, 2003-07), so Capcom had post-3.9.3 support patches, presumably including
the compiler build that produces differences #1-#9. If a later drop ever surfaces: build it with
tools/sn-gcc/build.sh into its own dir, smoke-test emwep `setThrow` (#1) and pl_class `isKamae` (#6),
then remove `// COMPILER-DIFF:` workarounds one at a time.

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
- Deferred inlines (`inline` members, in-class bodies, synthesized dtors) are emitted after
  `__static_initialization_and_destruction_0` in *definition* order (saved_inlines, oldest first): header
  in-class bodies in class order, then the unit's own `inline` definitions in file order, then the
  template instantiations finish_file requests. An implicit (synthesized) derived dtor is saved at the
  end of its class, before its in-class virtuals, and only instantiates the base `~cManager<T>` in
  finish_file (so those land last, in vtable-walk order); a user-written `~cMgr() {}` instantiates it at
  the definition point (game/model: `~cModInfoMgr, memAlloc, memFree, memClear, ~cParts, ~cPartsMgr,
  ...` needs the mgr dtors implicit and memAlloc/memFree/memClear in-class in model.h).
- A `static const` table referenced only by dead-stripped static functions is output by
  wrapup_global_declarations at the end of finish_file's first pass: between the last vtable of the first
  vtable group and a vtable needed only by a deferred inline (model: the 0x10 zero words between the
  cCoord and cUnit vtable copies = `static const s32 ShadowPtNum[4]` used by the dead ShadowModelInit /
  AddShadowModel).
- A byte store `x = 0` makes a QImode zero pseudo that later word stores cannot share; `U8Set(x, 0)`
  (u8& setter, promoted parameter) makes it SImode, and cse's skip-blocks path carries it over a
  one-statement `if` so the word stores after the join reuse the same register (modelInit).
- `Derived() : cUnit(1)` (base initializer) instead of `be_flag = 1;` in the body moves the constant's
  pseudo before the vptr pseudo and gives it r0 (cModelInfo::cModelInfo); the store order is unchanged.
- `for (i = 0; lim = n + 1, i < nArray - lim; i++)`: recomputing `n + 1` in the loop test reproduces
  the entry guard + `mr r7, r0` PRE copy; `u32 lim = n + 1` before the loop gives one register, and
  `nArray - (n + 1)` is reassociated by fold to `(nArray - 1) - n` (createSequential).
- Deferred-inline `inline` functions that other units call out of line (`isTrans__6cModel`) must stay in
  the .cpp: an in-class body would be inlined into those units.
- A dead static function's pool and strings stay in `.rodata` (STRIP_UNUSED drops the body); write it
  with the strings the target shows and reference the globals whose `.sdata`/`.data` slots it owns.
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
  Try this first on the id_sys/emobj/em_set OPEN cases. NOTE (option `setTime`): the original caller
  sign-extends (`extsh`) an argument whose callee is mangled `Us` (u16) — no cast form gives `extsh`
  for an unsigned parameter, so the original compiler extends narrow arguments at call sites in a way
  ours does not (likely a compiler-build difference in argument promotion, like the FPR/GPR arg-move
  order). Workaround: an asm-labelled signed alias (`setTimeS(IdUnit*, s16) asm("setTime__...Us")`).
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
- `cModel` (include/model.h) is 0x320 bytes; `cEm`, `cObj`, `cMap` start their own fields at 0x320
  (sizeof(cEm) 0xDE0, sizeof(cObj) 0x3D8, sizeof(cPlayer) 0xDE0, sizeof(cMap) 0x324 and all 391
  probed field offsets unchanged by the refactor, verified with an offsetof harness before and after;
  `cMotModel` is now an empty cModel subclass, 0x320 instead of 0x2B4). Layout (after cCoord's 0xF4 bytes):

  | offset | field | notes |
  |---|---|---|
  | 0xF4 | `pParts` / `pPartsHead` | cModel* / cParts* views of the parts chain |
  | 0xF8 | `serial` | |
  | 0xFC | `stat` / `xFC..xFF` | word / byte views |
  | 0x100 | `id`, `type`, `nParts`, `x103` | |
  | 0x104 | `speed`, `oldPos`, `wallNrm` | Vec ×3 |
  | 0x128 | `pFloorNrm`, `x12C..x12F`, `pCldShMd`, `shdCol`, `x135..x13B`, `fixParts`, `fixPos`, `x14C..x14F` | aliased by the obj05 `efmStat/efmSpd/efmRotSpd` view |
  | 0x150 | `x150` / `x150w` | f32 / u32 |
  | 0x154 | `alpha`, `x158`, `pInfo`, `pShMdInfo` | |
  | 0x164 | `lightInfo` | cLightInfo, 0x74 bytes |
  | 0x1D8 | `mot` | MotionWork (0xDC bytes); the cEm/cObj names (`pMotion`, `motFlags`, `motState`, `motFlags2`/`x21C`, `satPos`, `seNo`, `seFlags28B`/`motEvent`, `frame`/`motFrame`, `frameMax`/`motSeqMax`, `motSpeedRate`, `x29D`, `p2A4`, `blendMot`/`motBlend`, `motFlip`, `x2B0`) are an anonymous-struct view of it |
  | 0x2B4 | `atari` | cAtariInfo (0x4C); wrapped in an anonymous struct so no member ctor runs, cModel::cModel calls `AtariInfoConstruct` |
  | 0x300 | `x300`, `x304` | |
  | 0x308 | `pFootShadowTbl` | |
  | 0x30C | `litArea` | EmLightArea (0x10) |
  | 0x31C | `pTexChg` | cTexChg* |
  | 0x2B4 | `sub2B4` | ObjSub2B4: the object units' view of 0x2B4..0x320 |

  `MotionSeqKey/MotionData/MotionWork`, `EmLightArea`, `ObjSub2B4` now live in model.h; `cParts`
  (0x1D8, a cCoord with `pNext`, `bindMat`, `addRot`, `motParts`) and the two managers are declared
  before cModel. `cObj::blk` sits at 0x324 (was `sub2B4.blk`). `p2A4` is `void*` (cam_ctrl casts it).
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
- The `u8 pad[0x320 - sizeof(cModel)]` locals some units still carry are zero-length arrays now (a GNU
  extension GCC 2.95 accepts); they take no frame space and can be deleted when the unit is touched.
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
  recomputed. SOLVED (sscrn/mercenaries FadeSet colour temps, `&pos` for setAng): see "FadeSet colour
  pair" below — the fresh `addi rX,r1,ofs` per call comes from a local of an INLINED helper, whose
  frame address integrate.c substitutes straight into the hard-register argument sets.
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
- A value-context `a || b` inside an inline that returns it expands as `x = 0; if (!exp) goto L;
  x = 1;` (`li 0` first); `if (a || b) return 1; return 0;` gives `beq L0; li 1; b; L0: li 0`.
- A `switch` whose cases reassign the switched variable keeps the pre-switch copy (`mr r6,r30`) only
  if the variable is `int`; a promoted `s8` moves the copy to the variable's initialisation.
- Local `u8` array initialisers: 2 bytes -> `sth` immediate; 4 -> `stw 0` + `stb`s; 5+ -> `.rodata`
  template copy.
- Store-block order, refined: dying-source stores first as [last member of D in RTL order] + [rest of
  D in RTL order], then non-dying stores in RTL order; a byte store in the block is a barrier that
  splits it into two such groups.
- A constant kept in a callee-saved register across calls and stored later is a function-scope local
  (`int zero = 0;`, `int type = 2;`) stored through the variable.
- Float box tests with plain `blt/bgt` and a duplicated `blt` to the same label = separate
  `if (v.x < a) continue;` statements, the duplicated test copied verbatim.
- `.rodata` proves the include set: a unit whose `.rodata` lacks `"D:/Bio4/Prog/light.h"` did not
  include light.h; header strings appear in include order.
- Index register class: `add rD,rBase,rOfs` / `lwzx` with the offset in r9/r11 (BASE_REGS) instead of
  r0 means the base pseudo is not pointer-flagged — a `u32 addr` *parameter*; a `u32 addr = (u32)ptr`
  local does not work.
- `u32 trg = Key.trg & MASK` (u64 truncated) tests as 32-bit `andis.`; a `u64 key` exclusive check
  gives the `li 0; mr; rlwinm; or.` word-pair test.
- `mr rLong,rTmp; stb rTmp` (value stored and copied into a long-lived variable) = a reused block
  temp (`u8 c; c = sr[ptn]; mat.r = c; r = c; c = sg[ptn]; ...`); multi-set `c` blocks coalescing.
- An uninitialised `GXColor amb;` passed by value emits `stw rCalleeSaved, slot` (garbage register).
- Argument-move order workaround is per call site: a call may need the floats-first asm-labelled
  redeclaration while another call of the same function in the unit matches with the real one.
- flow.c appends `(use (const_int 0))` after any CALL_INSN that ends a block; jump2's cross-jump then
  fails against the fallthrough, so one of N identical call tails stays unmerged unless the arm does
  not end in the call (repeat a trailing store in every arm to let the tails merge deeper).
- A pointer local assigned in two blocks is not local-allocated: a struct copy through it keeps the
  `addi` untied from r3 and following loads are scheduled after the copy.
- `int atk; w->atkHit = 0; atk = 0;` (assignment after the byte store) keeps the QI and SI zeros in
  separate registers and makes cse pick `atk` as the 0 stack argument of later calls.
- `if (cond ? f() : g())` duplicates `cmpwi r3,0` into both arms; `hit = cond ? f() : g(); if (hit)`
  gives the single shared compare after the join.
- `0xFFu` (unsigned literal) in a ternary makes the following compare `cmplwi` rather than `cmpwi`.
- Odd float constants: `0.05f` in the target is `0x3D4CCCCC`, obtainable only as `0.01f * 5.0f`
  (constant-folded) — when a pool word is off by one ulp, look for a folded product.
- `HALT()` (and any OSReport-style error macro) is a plain `{ }` block in the original, not
  `do{}while(0)`: the loop notes of a do-while are a full sched1 barrier and change argument/`lis`
  ordering around it (read, main_sub, pl_leon, datactrl needed the plain form; nowhere did do-while
  help). Use the plain block everywhere.
- A function with an `if (...) return 1;` and no return at the end keeps r3 = the incoming
  parameter (no `li r3,1` in the tail).
- `pSys->field` inside loops that store through a pointer parameter is hoisted (fixed scalar never
  aliases a varying struct store); reading it through a reference (`SysRef(pSys)->x`) reloads it per
  iteration like the target.
- objdiff REPLACE rows on `lwz/stw off(rN)` with identical bytes = dtk-synthesized `Sym+off` relocs in
  the split object; judge with a raw byte compare (relocated words masked).
- Sprite texture-corner selection (esp_sub/esp08/esp18/esp0f/esp16 Trans): one condition
  `(screen && !f4) || (!screen && f4)`, corners built from a `zero` variable with the flip-s leaves
  adding first (`s0 = zero + z; s1 = zero;`): each leaf is a jump target with no cse knowledge of
  `zero`/`z`, which keeps the `fadds` (nested ifs with literals fold `0 + z`). Fixes the family-wide
  `fadds` vs `fmr` diff.
- Two identical calls in if/else arms are not cross-jumped when the shared argument is a local read
  before the `if`.
- Switch nodes sharing the default target: a leaf whose every exit goes to default vanishes (grouped
  with `default:`), but a default-target node with a right sibling in its chain keeps `cmpwi N; beq
  default`; both count for the tree balance.
- `u8 stat = 1;` gives a QImode pseudo folded into the `stb` (late `li r0,1`); `int one = 1` shared
  between a u16 call argument and a u8 store keeps an SImode pseudo hoisted into a callee-saved reg.
- Read constant pools from the split `.o` bytes, not by hand-parsing the `.s`; objdiff scores pool
  values symbolically and hides wrong constants.
- `INDIRECT_REF(PLUS)` marks a MEM in-struct; the same address through a `(u32)` cast does not:
  `*(u16*)((i << 4) + u32helper(ofs))` gives `sthx base,idx` and a `pG` reload after every store.
- A redundant second assignment (`parent = w->pParent;` again) makes REG_N_SETS=2 and stops regmove
  from merging `lwz r0; mr r29,r0; cmpwi r0`.
- Float `ble/bge` without `cror` = reversed `>`/`<`: write `if (!(a > b))`; `cror un,eq,lt; bso` is
  the real `<=`.
- Work-struct init blocks: int/f32 fields written through reference setters (`ISet/FSet`) while u8
  fields are plain stores gives the target's store schedule where plain int stores never do.
- `insert_bct` refuses known loop counts < 3: a 2-iteration loop only becomes `mtctr/bdnz` if the
  count is not visible to loop.c.
- Routine bytes: the `ff, fd, fc, fe` store order of an `xFC/xFD/xFE/xFF` state change comes from an
  inline `PlRoutineSet(pl, int, int, int, int)` storing `fc, fd, fe, ff`; direct byte stores give
  the dying-first order.
- A static local aggregate with a `_.tmp_0` guard and per-member `stfs 0.0` is a class with a
  constructor (`struct P { f32 x,y,z; P() {..} }`), not a POD initializer.
- `register int r4v asm("r4"); int mode = r4v;` at entry reproduces reading an argument a
  parameterless mangled name does not declare (`beginEvent()` reading r4).
- `void f(...) asm("f__6cBase...");` in a derived class re-exposes a hidden base overload without a
  body; `extern T* alias asm("sym");` gives a second name/type for a conflicting global.
- Interblock scheduling threshold: haifa's `find_rgns` makes a loop one region only if its LUID span
  is <= 100 (notes and deleted insns count). A loop body over that limit shows no speculative
  hoisting (`mcrf cr7,cr0`, arg `li`s before the branch).
- No `clrlwi` for `0x40 + i` passed to a `u8` parameter is only obtainable with an int-parameter
  asm-labelled alias of the callee (`unitPtrI`, `setI` in id_sys.h).
- Stores to a plain `static void*` are freely reordered against `u->member` loads; declaring them as
  one-element arrays (`g_p[1]`) keeps the target's load/store interleave.
- `psq_l f,0(rP),1,qrN` straight from a stepping pointer is inline asm; the compiler always goes
  through a stack slot. Whole skinning kernels (`CalcSk1_x`, `setupGQR6`) are single `asm volatile`
  bodies (NgcAs rejects `subis`; use `addis 0xE000`).
- Locked-cache palette: `PSMTXReorder(m, (f32(*)[3])(0xE0000000 + i*0x30))` gives `mulli; subis`.
- `cond ? A : B` as a call argument folds a common `(x+0x1F)&~0x1F` out of both arms; two if/else
  calls keep the arm-specific masks and cross-jump only the `bl`.
- loop.c `move_movables` needs `threshold * savings * lifetime >= insn_count`: in very large loops
  (>~300 insns) a lo_sum with savings 2 is not hoisted by ours while the original hoists it — the
  original's loop was smaller or its invariant had more uses; count the loop insns before guessing.
- `&local` recomputed per call requires the local to be the first declared frame object (offset 0).
- `lwz r0,X; mr r3,r0; cmpwi r0` on a struct member = the member read directly in several blocks
  (gcse reaching copy); a local gives `lwz r3` directly.
- `ang = w->rotY; ang -= K;` (two statements) loads straight into the variable's register;
  `ang = w->rotY - K` gives a temp-first order.
- A switch whose arms each store to `pG->field` gets the `pG` reload PRE'd into a copy (`mr r11,r9`)
  and the arms never cross-jump; a local temp with one store after the switch gives the merged form.
- A shared scalar temporary across a run of `p = load; store(f(p))` statements reproduces the
  one-scratch-register interleaved schedule; distinct expressions get all loads hoisted.
- Locals declared after a call (`int x = 0;` following `OSReport(..)`) keep their `li` after the `bl`.
- Extern declaration order decides `.sbss`/`.bss` placement of *referenced* externs (first-declaration
  rule): `extern GameSaveData* pSaveData;` must precede `extern cGameSave GameSave;` in game.h.
- A loop pointer initialised by an `mr` copy of the hoisted `&Array` pseudo comes from a dead
  initializer `T* t = g_Arr;` at function scope with `t = &g_Arr[i]` in the body (the extra mention
  keeps the base pseudo live past the giv init).
- A zeroing loop over a word array steps up with `mtctr` only with a `u32` counter; `int` reverses it.
- `>= C` / `< C` compares survive only when the constant is not visible to fold: a plain local
  (`lim = 30; if (x < lim)`) keeps `cmplwi 0x1e; bge`; a literal is canonicalised by combine.
- `if (c) x = a; else { x = b; ... }` hoists `x = a` above the test only when the else arm *starts*
  with `x = b`; an else arm starting with a call keeps it in place.
- A function ending with `return 0;` makes every early `return 0` jump to that final `li r3,0`.
- C frame slots (cc1): BLKmode locals get their slot at declaration, rounded to 8; address-taken
  sub-word scalars are put in the stack at the first `&` in parse order; word-or-larger ones go
  through ADDRESSOF and get slots at purge time, so they land after every sub-word slot.
- `x == 0 && y == 0` on adjacent `short` struct members folds into a single `lwz; cmpwi 0`.
- `int one = 1;` at function scope with a single use in another block: `update_equiv_regs` moves the
  `li` next to the store, making a short qty that takes r0 ahead of a `lwz/rlwinm/stw` chain.
- `if (a && b) {..} else if (c && d) {..}`: the else-if test block has two predecessors, so cse cannot
  reuse cr0 and gcse PREs the shared member load (`lfs f0; fmr f12,f0`); nested ifs share cr0.
- Identical switch bodies written separately are cross-jumped into the *last* copy in source order.
- gcse cprop of a `p = A` copy (A = an inline's parameter pseudo) is blocked by *any* other set of `p`
  on a path; an in-loop `p = &x->m;` re-assignment is such a set, gets hoisted by loop.c and deleted by
  cse2 as a no-op — reproduces a lone `mr rX,rA` copy. gcse runs one pass (MAX_PASSES 1).
- After sched1, REG_LIVE_LENGTH counts only real insns: block notes/braces cannot shift global-alloc
  priorities; only real insns in the range do.
- jump2 cross-jump deletes the tail of the jump scanned *first*: to keep an if/else then-block in
  place and have identical case bodies jump into it, write the case body arm before the if/else arm.
- Early-return tests that branch to a *later test* (not the function end) mean the original nested
  the ifs, not `if (!a) return;` chains.
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
- FadeSet colour pair (sscrn, mercenaries; `include/fade.h` `FadeColorPair`/`FadeSetW`): the recomputed
  `addi r4, r1, 8; addi r5, r1, 0xc` before every `FadeSet` and the fresh `addi r9, r1, 0x68` for
  `setAng(&pos)` after `setPos(&pos)` (OpeSetOpenTerm, `PlSetPosW`) are the locals of an INLINED helper.
  Mechanism (`.rtl`/`.gcse`/`.greg` dumps): the caller's own `&local` is a `(plus vsv N)` that
  `precompute_register_parameters` copies to a pseudo, which cse/gcse then merge and PRE hoist. A local
  of an inlined `static inline` function lives in the inline's frame; integrate.c maps that frame to a
  pseudo P with a `REG_EQUIV (plus vsv N)` and cse substitutes the constant address straight into the
  hard-register arg sets (`(set r4 (plus fp 8))`), which `hash_scan_set` never enters into the gcse table
  (hard-reg dest) — so the address is recomputed at every call and never PRE'd. Rules: (1) the colour pair
  must be ONE local of the inline (`FadeColorPair col` with a user copy-ctor so it is BLKmode and every
  inlined copy shares one 8-byte slot; two `GXColor`/`u32` locals get fixed spilled slots); (2) a store of
  a CONSTANT through P is rejected by recog (no store-immediate on PPC), which cancels that substitution
  group and keeps P (`stw rZ, 4(rP)`, `mr r4, rP`, PRE'd) — so write the constants through a value that
  was set before a label (`black = 0xFF` set once between the two `if`s, both branches of an `if (no &
  0x80000000)` for the 0/0xFF choice), which is why `FadeSetW` looks the way it does; (3) with the local at
  frame offset 0 the frame register itself is substituted and everything is direct. Same lever for any
  "second `&local` is fresh per call while the first is `mr r4, r30`" case: wrap the call in a
  `static inline` helper that takes the address by pointer parameter (integrate substitutes the caller's
  `&pos` for a read-only parameter) or owns the local. penClothAtMake's `&v1`: keep the header
  `PSMTXMultVec(..., &v1)` inside the inline (`penPartsWorldPos`) and make the *case bodies* use one
  `Vec* pv1 = &v1` pseudo declared after the header calls, so gcse has a pseudo to hoist (`addi r26, r1,
  0x18` in the prologue) while the header call stays a fresh `addi r5, r1, 0x18`.
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
- `cAtariInfo::init(int,int,int,f32 x7)` `fmr`/`li` order (SetTrolley/SetGondola/SetYagura/SetHeliMissile/
  SetPillar/SetBox/SetBarrel/SetEmSwitch/setScrAtari, also Espgen44_Destruct's `Filter05SetParam`,
  emobj's `SatMgr.create`): NOT adjust_priority/birthing — SetYagura has no loop, >10 blocks and >100
  insns, so every block is its own region and `bb_live_regs` never holds f1..f7 there. The `.sched`/
  `.sched2` dumps show the seven FPR arg copies and the three `li`s with equal priority (7), equal
  register weight (+1, nothing dies: the `zero` pseudo lives on for `pos = 0`, the copied hard regs f2/f5
  are call args) and equal dependence count, so the ready list falls through to INSN_LUID: the target
  order (`mr r3,this; fmr f1; fmr f6; li r4; fmr f7; li r5; li r6`, fpu one insn/cycle, sched2 keeping
  sched1's order) is exactly what an RTL order "this, FPR args, GPR args" produces, and GCC's
  `load_register_parameters` emits the moves in declaration order (ints first). Proven by an asm-labelled
  redeclaration with the floats first (`include/atari_init.h`: `atariInitF(cAtariInfo*, f32 x7, int x3)
  asm("init__10cAtariInfoiiifffffff")`, ABI-identical since GPR and FPR argument registers are numbered
  independently); with the float constants passed through an inline (`AtariInit`) so they are pseudos
  (cse shares the 0.0 with the `pos = 0` stores through the `if (pos)` branch; the 1000.0 pseudo feeding
  both f2 and f7 gets the longer chain the target loads first). The same "FP arg moves before GPR arg
  moves" appears in every unmatched int-then-float call site checked (embarrel/emBarred/emrock init,
  emobj `create(..., int, int, f32)` with `lwz`/`lfs` args) but not for GPR args that are register copies
  (`PSVECScale(&v, &v, f)` keeps `mr r3; mr r4; fmr f1`): a `calls.c` experiment that always emits arg 0,
  then the FPR args, then the other GPR args flipped 13 functions and regressed 37 (all cases with
  register-copy/`addi`/symbol GPR args after an FP arg), so the exact rule of the original compiler is
  still unknown; use the redeclaration where the target shows the interleave.
- A static function's position in `.text` is its definition position: `objTrolleySatClear` is defined
  after `objTrolley_R0_Break` in the original (forward-declared before `move`), which objdiff's 100%
  does not show — check `.text` symbol offsets against the split object before flipping.
- A store block's weight-0 member is the one where the *work pointer* dies (its last use in the block):
  SetBox's `w->itemNo = -1` is the last `w->` store in source although the target issues it first
  (both `-1` and `w` die there); with it written first, `w` died at `itemNum = 0` and that store jumped
  ahead instead.
- A distance test computed into the function's existing `f32 spd` variable (`spd = dx*dx + dz*dz; if
  (spd < K)`) lands in f1 (global pseudo, two assignments) where the inline expression ties `fmadds` to
  the dying operand's f13 (embarrel emBarrelSetRollSpd).
- An inline helper taking `const Vec* size` evaluates `&size` as the parameter copy before the body's
  `&ofs`, reversing the `lis/addi` pairs of `init2(0, 1, &ofs, &size, 0x10)`; a helper that *returns*
  `&ofs` (shared `.rodata` copy) used as the argument keeps the argument order (embarrel).
- (emrock) `cAtariInfo::init` with *computed* float arguments (`em->scale.x * 1200.0f * 0.5f`): plain
  `atariInitF(&em->atari, ...)` with the constants written inline reproduces both arms (the `fmr f5,f4;
  fmr f6,f5` chain and the `fmuls`-interleaved `li`s); `AtariInit` (pseudo constants) does not. The
  `&em->atari` pseudo PRE'd into r28 for the later `setPriority`/`clrFlag100()` comes from the member
  call form, so use `em->atari.clrFlag100()` (not `em->atari.flags &= ~0x100`, which re-derives the
  address from `em`).
- Any int-then-float call whose target issues the FPR moves before the `li`s can be redeclared with the
  floats first under `asm("<mangled>")` (atari_init.h idiom): `setYarareCubeF(cEmRock*, f32, f32, f32,
  Vec*) asm("setYarareCube__7cEmRockP3Vecfff")` fixes `fmr f3,f1` before `li r4,0` in setFall/setThrow.
- A callee whose mangled name says one parameter but whose body reads r5 (`cGameSave::save(void*)` reads
  an `int` in r5; every caller loads it) is declared through a free asm-labelled function with the real
  parameters *and the real return type*: `int GameSaveSave(cGameSave*, void*, int) asm("save__9cGameSavePv")`.
  Declared `void`, `li r3, GameSave@sda21` is issued before `li r5, -1`; with `int` the r3 output
  dependence puts it last (the "callee return type" rule also applies to asm-labelled aliases).
- `dx*dx + dy*dy + dz*dz` compared with a radius sum: compute `len` into its variable first and
  `r = w->radius + 1000.0f` *after* it (`fadds` then `fmuls f0,f0,f0` tied), then `if (len > r * r)`;
  with `r` computed before `len` the radius load is interleaved into the distance chain (emrock DropHitCk).
- `if (atk) { ...; if (EmAtkHitCk(...)) { ...; return 1; } } return 0;` gives both tests `beq` to one
  `li r3,0` block at the end; two separate `return 0`s put `li r3,0; b end` after the second test.
- `pl->frame / (f32) pl->frameMax` (u16 member, `psq_l qr3`) times a u16 motion count held in a `u32`
  local (`u32 cnt = hdr->maxFrame` → unsigned double trick) converted with `(u32)` gives the
  `fcmpu 2^31/cror/bso` unsigned conversion; write the ratio into its own `f32` first when the target
  computes it before the count (plemRockEscape).
- `(int) pl->x3E0 / 20` on the `u32` cEm field gives the signed `mulhw 0x66666667; srawi 3` divide (obj13
  uses the same `(int)` cast for signed tests; do not change the shared field type).
- A pointer local `Camera* cam = &G;` + `FSet(cam->param.fovy, C)` keeps the store `stfs 0xc0(rCam)`
  and, being a scalar-reference store, gives it a dependence on the following `lwz pG` so it is issued
  before the `addi r5, r1, 8` argument; later `&G.param.at` written on the *global* are cse
  related-values `addi r5, rCam, 0xb0` recomputed per call (a `cam->param.at` pointer form PRE's them
  into callee-saved registers). In a block that follows a branch join the same `&G.param.at` is the
  `lis rH, G+0xb0@ha` / `addi rX, rH, G+0xb0@l` pair with the high part shared (emrock cam functions).
- OPEN (emrock plemRockEscapeCamMove2 / plemRockDropDieCamMove tails): after `PosToPos(..., &G.param.at)`
  calls, the tail's `Vec* cp/ca = &G.param.pos/.at` reuse the calls' high pseudos (`addi r9, r29, G+0xa4@l`)
  and `Camera* cam = &G` is a *fresh* `lis/addi` pair; ours relates `cam` to the newest pointer
  (`subi r30, r9, 0xb0`). When the pointer comes from a struct copy with its own fresh `lis`
  (emRockPushCamMove) the `subi r30, r9, 0xa4` form is what the target has. The cse related-value
  chain needs the lo_sum to fold, which requires the high pseudo to be known in the tail's ebb; no
  source form found that hides it (~10 tried). Also OPEN: SetRock's `w->seAlways[2] = 0` (last QI use of
  the shared zero) is issued in source order by the original although the register dies there.
- A zero word in `.rodata` between two functions' pools that no code references is the pool of a
  dead-stripped `static` function (emrock: `static void emRockSpdClear()` with three `= 0.0f` stores
  between setThrow2 and setYarareCube, unit added to `STRIP_UNUSED`); compare the `.rodata` words of
  the compiled object against the split object directly, objdiff does not see it.
- global.c allocation priority is `floor_log2(refs)*refs/live_length` *truncated to an int* (`*10000`),
  ties broken by pseudo number. REG_LIVE_LENGTH counts every insn *and note/label/barrier* in the range
  (flow.c increments outside the `'i'`-class test), so deleted statements, block notes and jump layout
  shift it. pl_leon setRightHand: `data` (5 refs/20) lost r30 to `info` (5 refs/19) only because the
  do-while(0) HALT let cse rewrite the HALT store's 0 as `info` (a 5th ref); the plain-block HALT gives
  info 4 refs and `data` wins. Hoisted invariants that never die (loop-invariant `lis` in a `for(;;)`)
  all truncate to the same priority and are allocated in pseudo-number order.
- `const f32 name = literal;` locals: the initialiser is expanded (creating the pool entry at that
  point and a 4-byte frame slot) but emits no code, and every use is folded to the literal, so `h +
  name` is computed where it is used while the pool order follows the declarations. pl_push emSandCheck
  needs `const f32 sand = 800.0f; const f32 base = 300.0f;` (pool 800, 300, 0.0 with rot stored first).
- Zero stores come out in pure source order (none first) when no store is the zero's last use: the
  dying one is the *last* zero store in source (`x54 = 0` after `flags = 0`, pl_cloth testDressSetAda).
- Two different QI zero stores separated by a call reuse one pseudo (callee-saved); the original's
  fresh `li r0,0` after the call = the second group used an SImode zero: `u8` fields stored from `int`
  values (an inline `PlRoutineSet(pl, int, int, int, int)`), cse cannot merge SI and QI zeros (pl_dmg).
- `do { } while (0)` macro bodies emit NOTE_INSN_LOOP_BEG/END; haifa makes the first insn after a
  loop note depend on *everything* before it (loop_notes → full barrier), so PRE copies inserted at
  the block end cannot move above the next macro invocation. Written-out blocks with a plain `{ }`
  (pl_cloth LAPEL_MOVE) let the copies schedule right after their `addi`s; re-deriving `pm1 = m1`
  inside the inner block makes the first lapel's later uses go through the PRE copy (`mr r24,r27`).
- PRE-created pseudos are numbered in *hash bucket order* (pre_delete walks `expr_hash_table`), with
  `hash_expr` hashing `high(symbol_ref)` by the symbol *name* (so `.LC<n>` numbering and the table
  size `max_uid/4|1` both matter); the insertions themselves come in bitmap-index (first-occurrence)
  order. path PathGetMatEm: three dead `int x = 0;` initialisers (+3 uids) plus taking `&hpos.x[j]`
  before `&key0` in the loop body give the target's spill slots and copy order. exception ErrorHandler
  (LC64/symbol_err_tbl swap) has no solution with our `.LC` numbering — the original TU numbered its
  constants differently (open).
- gcse 2.95 PRE of a struct load across an if/else: to stop it, kill memory at the *join* (an empty
  `asm volatile("" : : : "memory")` as the first statement after the if/else makes the join's loads
  non-anticipatable; placed inside the arm it leaves the conversion paths jumping to the asm's label).
  A `default:` arm that falls into `case 0:` gives its string `lis` an extra anti-dependence on the
  `err` call (the call's `depend_count` tie-break then issues `lis` before `lwz pLog`); a `default:`
  with its own `wrap = 2; break;` (cross-jumped) keeps `lwz pLog` first (TexRender CopyTexRenderMgr).
- Reusing a dead pointer local for a later block (`p = &tile[0]; ... p = &tile[1]`) extends its live
  range to the function end and gives it the top callee-saved register (datactrl dispDebug p=r31).
- A variable set in the two paths of an unsigned float→int conversion is global-allocated; a second
  such variable in another block must be a *different* local (block-scoped `u32 x0`) or the shared
  pseudo's range covers both (datactrl loop x0 r6 vs over-block x0 r5).
- `int n = 0; int base = 0;` declared inside an `if` block (C++ mid-block) put their `li`s in that
  block next to the calls; at function scope they are hoisted to the prologue (stage subMissionSt1).
- `SatMgr.destroy(p)` on the object is a direct `bl destroy__7cSatMgrP4cSat`; `sat->destroy(p)`
  through a pointer is a vtable call (pl_debug satMakeTest) — check when a manager method is virtual.
- objdiff REPLACE rows with identical text: the split object can carry a synthesized `R_PPC_NONE`
  reloc (path `lfs f12,0(r29)`) or a symbol+addend spelled from a neighbouring symbol
  (`globalCamera+0xe0` = `g_RndMgr-0x38`); compare bytes/addresses, not the row.
- `x == 2 || x == 3 || ... || x == 6` on one lvalue is range-folded by fold_range_test, and five
  separate `if (x == k) return 1;` make the last test a setcc (jump.c store-flag on a single-use
  diamond). The unfolded chain (`cmpwi k; beq L1` x4, `cmpwi 6; bne L0; L1: li 1; b; L0: li 0`) is a
  `||` of inline CALL_EXPRs: `SysRegionIs(2) || SysRegionIs(3) ...` with
  `static inline int SysRegionIs(int r) { return pSys->region == r; }` — operand_equal_p refuses
  expressions with TREE_SIDE_EFFECTS, and the shared true-label sits between the last `bne` and the
  `li 1` so jump.c cannot make a setcc (dvd SysIsEurope, card has the same chain).
- cse-follow-jumps decides `lwzx rD,rIdx,rBase` (index form) vs `lwz rD,0(rSum)` for `*ph` with
  `ph = &pFilehead[depth]`: the sum register is replaced by the index form only inside the extended
  block that knows the equivalence, i.e. blocks entered through a once-used label preceded by a
  barrier (cse follows them TAKEN, then re-runs the fall-through). A block that starts with a label
  entered by a `goto` from elsewhere (`snd_err:` shared by two error paths) is a fresh ebb and keeps
  the sum form; the original duplicated the error block in both `if (r == -1)` arms and let jump2
  cross-jump them (dvd readMain, also flips the r28/r29 allocation of ph vs &pFilehead).
- The `mr; cmpwi` vs `mr.` question (mes) is a combine question: `P = r3; cmp P` right after a call
  always fuses into `mr.` in our RTL; the copy survives unfused only when can_combine_p fails
  (a set of r3 or a volatile insn between them, a label, or a non-REG dest). ~30 forms tried
  (int/u16/s16/u32/volatile/register locals, switch, goto, `if ((code = f()) ..)`, inline wrappers
  give `rlwinm; cmpwi`). Still open.
- A do/while(0) HALT macro is a sched1 barrier (loop notes): with a plain `{ }` HALT the preceding
  `pLog->err`/`OSReport` argument moves are ranked together with HALT's own `lis r4,__FILE__`,
  which pulls `li r4,0`/`lwz r4` before the string `lis/addi r3|r6` (main_sub DLL_Link/DLL_Unlink,
  read decodeData/ReadPlayerData/ReadWepData — the ReadPlayerData `mfcr` OPEN went away with it).
  Units where the do/while form matches (eprintf, sce_com) keep it: try both per unit.
- The `mr rN,rM` copy of a just-loaded global (`lwz r0,g; mr r7,r0; cmplw r0,..`) means the compare
  read the global *before* the local was assigned: `if (mess_keep_ptr >= ..) return; p = mess_keep_ptr;`
  — cse turns the second load into a copy of the first (eprintf EprintfBuffering). Also there:
  `h / 1.3333333f` (7 digits) is 0x3faaaaaa; the original constant is 0x3faaaaab (`1.33333333f`).
- ErrCheck-style `pMes`/`pStr` r20/r21 swaps between two 3-ref invariants: priority = int(30000/L);
  L 390 vs 386 fall in buckets 76/77, so the later-declared one wins; the original's lengths must
  share a bucket (then the lower pseudo wins). Block-scoped declarations and do/while notes do NOT
  change REG_LIVE_LENGTH at global-alloc time (only real insns count there).
- Asm-labelled *definitions* (`int IDSystem::setCkI(int) asm("setCk__8IDSystemUc")`) do not
  assemble: SN's cc1plus emits the function-begin label as `.L_f*setCk__8IDSystemUc_s` (the `*`
  of the asm name) and NgcAs rejects it. So the narrow-parameter masks (`clrlwi rP,rP,24` on a u8
  parameter, id_sys setCk/dispSw/kill) cannot be reproduced by an int-parameter view; an explicit
  `type & 0xFF` is folded away too (nonzero_bits knows the promoted parameter). Only a tool-side
  rewrite of that label (or a compiler flag for argument promotion) would open these.
- gcse PRE copies land at the *end of the block* that computes the expression, and in C++ a block
  ends at every call (EH: flow appends `use (const_int 0)` after CALL_INSNs). An `&member`/`&local`
  address computed in both arms of an if/else and used after the join therefore gets its copy
  `mr r28,r29` right *after* the arm's `bl` (dvd Initialize: `sprintf(name, ..)` in both arms, then
  `name` used directly after the join); a source-level `n = name;` is a cse copy at the statement
  that sched1 hoists above the call. Write the global/member directly after the join, no local.
- `lwz r9,g; <use r9>; mr r11,r9` with later blocks using r11 = the global read directly in a block
  that cse cannot reach (a join with two predecessors after `a && b` / `a || b`, or a loop test):
  gcse PRE re-loads it at the end of bb 0 and cse2 turns that into a copy whose register becomes
  canonical (last use beyond the ebb) for every later block. A local `T* p = g;` merges all reads
  into one register and hides the copy (pl_wep getAngle/getPitch `pPL`, scroll smxInit `pSmx`,
  getWorkNum's loop bound `grp.nGroup` with a guarded do-while + `grp.num[i]` indexing).
- A `li rY,C` loop counter issued *after* loop.c's hoisted `addi`/`lfs` in the preheader is a
  reversed count-up loop (`for (i = 0; i < 3; i++)` with `i` unused in the body); `for (i = 3;
  i != 0; i--)` puts the `li` at the statement position before the hoisted insns (pl_wep
  wepSetWaterShot).
- `(u8*)this + n*4 + 0x14` keeps `add this,idx; addi 0x14`; `(u8*)this + 0x14 + n*4` and
  `&member[n]` fold the constant into the index (`addi idx,0x14; add`) (scroll getWorkPtr).
- `if (a || b) return 0; return p;` shares one `li r3,0` block after the second test; two `if`s
  each returning 0 give two `li r3,0; blr` tails (cManager getPrevWork). `if (!(f & 4)) return NULL;
  return call();` keeps `li r3,0` in its own arm after the call arm; `flag ? call() : NULL` and
  `if (flag) return call(); return NULL;` let jump.c hoist `li r3,0` above the branch because the
  fallthrough arm then starts with a set of r3 (scroll SmdGetGroupNext).
- A struct-member load reloaded after a store to an address-taken stack local (`lwz r9,0x15c(r3)`
  twice around `stw r0,0x28(r1)`): the load was a reference read (`PRef(scr->pInfo)->pTpl`) — a MEM
  with neither struct nor scalar flag conflicts with the fixed-address scalar store, while a plain
  member read (in-struct vs fixed scalar) is hoisted/merged (esp_efm EfmSeqSet `model`/`tpl`).
- OPEN (dvd DiscChange): the 16-byte `game[]` template copy loads words 0,8,c,4; sched2 gives our
  word-4 load priority 9 (its `stw r9,4(r11)` is anti-dependent on the later `lwz r9,pSys`) so it
  goes first; declaration order, `char company[3]`, `const char* const`, separate stores all tried.
- OPEN (read readEmData): `newSize` (6 refs / 54 insns → 2222) beats `m` (14 refs / 198 → 2121);
  `MEM_ALLOC(size)` vs `MEM_ALLOC(newSize)` makes no difference (cse rewrites `size` to the
  later-used `newSize`). Needs `m` ≥ 15 refs or ≥ 4 more insns in newSize's range.
- OPEN (esp_efm EfmSetObj04, emrock emRockRollStartCk, route_ck RouteCkPosToPosDis): the target
  re-reads `w->x79` (`lbz r4`) in the else arm after `stw r24,0x6c(r31)` although cse1 following the
  `beq` merges it in ours (`-fno-cse-follow-jumps` reproduces the reload but is not a per-unit
  option); emrock's `mr r11,r9` pG copy has no third read to PRE; RouteCkPosToPosDis keeps
  `mr r3,r31; mr r4,r29` before `rckLineHitCheck(from, to..)` where reload_cse deletes ours
  (nothing sets r3/r4 or a label between the prologue and the call; goto/flat-if forms tried).
- Sprite corner leaves (esp.h `ESP_SPRITE_CORNERS`): the `!flip-s, flip-t` leaf adds into `s1`
  (`s0 = zero; s1 = s0 + z; t1 = s0; t0 = s1;`) in esp_sub/esp0f, into `t0` in esp08 — read the
  target's `fadds` destination per unit. With the idiom esp_sub Shimmer/Nega and esp0f are 100%.
- `x = 1; if (f() != 2) x = 0;` store-flags to `xori/subfic/adde` (jump.c: `reg_set_last` finds the
  constant 1 across the call, BRANCH_COST 0 case "A is a power of two, B is 0"); the reversed test
  `if (f() == 2) x = 0;` keeps `cmpwi/bne/li`. esp_sub Shimmer's `GetDrawTmpBufType() == 2` was
  simply the real condition (esp18 already had it).
- `EspSeqSet(rec, info, seed, model, mtx, int a, f32 f, cEsp** out, EspSeqOpt* p8, Vec* pos)` is the
  real parameter order (prologue `mr r24,r8; fmr f31,f1; mr r25,r9; mr r28,r10; lwz r23,0xc0(r1)`),
  callers are ABI-identical; its locals are `Vec v; Mtx m; Mtx m2; EspPtr e;` with the one `Vec`
  reused for the RotMatrixZXY input and the PSMTXMultVecSR output (frame 0xb8).
- Two constant-pool `lis` in one store block (0.0 vs 1.0 highs, EspCommonTrans' third arm) are
  issued in the RTL order of the first statement using each constant: `mat[2][2] = 1.0f` written
  after the first `= 0.0f` store puts the 0.0 `lis` first.
- A `lis rX,0x4330` in a callee-saved register far above the int→float conversions that use it
  (esp16 `Esp16_Trans`: `lis r31` after CameraCurrentProjection, `stw r31` in both if/else arms) is
  cse canonicalising the arms' constant pseudos to an OLDER one on the ebb path: a dead conversion
  earlier in the function (`rate = (f32)(int) n;`) whose result flow deletes; signed vs unsigned
  matters because only the SI constant is shared (the DF magics differ). update_equiv_regs moves a
  single-use constant next to its use, so the hoisted `lis` needs ≥ 2 live users (esp12's single
  conversion cannot be reproduced that way, still OPEN together with the `lfs f12; fmr f29,f12`
  copy for `t = 0.0f` in both units).
- Constant pool order via a dead declaration initialiser: `f32 ang = (f32)(int) esp->cnt;` (esp18)
  puts the signed DF magic before the `0.0f` of the next initialiser; flow deletes the load.
- `Filter05SetParam` argument-move order is per call site: all-immediate (Espgen44_Destruct) wants
  the floats-first alias, loaded arguments (Espgen44_SetFreeWork) want the floats between the 5th
  and 6th ints (`(int a,b,c,d,e, f32 x,y,z, int f,g) asm("Filter05SetParam__Fiiiiiiifff")`), the
  only one of 7 interleavings tried that matches — espgen44 is Matching with both.
- espgen02_Update (OPEN, corrected): both ours and the target load the shared 0.0f into spdR and copy
  to scaleR/colR; the diff is only spdR/colR = f24/f23 vs f23/f24 (global-alloc priority, spdR has
  5 refs vs colR's 4 in ours).
- `tools/fdiff.py` can fail with "Invalid control character" on units whose objdiff JSON contains raw
  bytes (esp_sub Shimmer): read with `json.load(..., strict=False)`; concurrent fdiff runs share
  `build/G4BE08/fdiff.json`, so a private copy with its own output path is safer.
- (player) cse's extended block ends at a label with two uses: `if (k & 4) {..} else if (k & 8) {..}`
  (join label used twice) makes the `&Key` address after the following calls a fresh `lis/addi`,
  while two separate `if`s let cse carry it in a callee-saved register (pl_R1_Run). `u64 key =
  Key.on;` tested twice keeps the low word in a register (`rlwinm r10,r12` without a reload);
  `Key.on & bit` twice reloads both words after the store between them.
- jump.c store-flag: every if/else or `x = !(...)` form of `if (c5 && f60 >= 0) moved = 0; else moved
  = 1;` becomes `srwi r26,r0,31` (the hoisted `x = 0` + `if (c) x = 1` diamond, A=0/B=1). The
  original's `li r26,0; cmpwi; bge L; li r26,1` needs a CODE_LABEL between the jump and `x = 1`:
  `if (c5) { moved = 0; if (f60 >= 0) goto ok; } moved = 1; ok:` (the label shared with the outer
  failure path blocks the pattern; cPlayer::move).
- A `u32 frame` local converted with `(u16) frame` at its uses gives one `clrlwi r30,r6,16` after
  the if/else join (Walk/Run motionSet + neck init); a `u16 frame` local gets the extension folded
  per branch (`lbz r30` / `li r30,0` / `clrlwi` inside the float arm).
- Dying-zero-store promotion: the zero store that comes first in the target's block is the *last* in
  source (init1: `satCheckFlag = 0` after `boss0 = 0`); the QI zero of an early `u8 = 0` stays its own
  pseudo (`li r0,0`) only when it precedes every SI zero store.
- Objects with constructors (`cMot3 mot3`, the `m3r` rates) are emitted at their definition, not
  deferred like plain uninitialised globals: define them after the function whose static local
  precedes them in `.bss` (player.cpp: after pl_R1_Turn180's `dd0`). A `f32 x[3]` whose static
  initializer stores one pool 0.0 three times (`stfs 0,m3r@l; stfs 4; stfs 8`) is a class with a
  constructor `r[0] = r[1] = r[2] = 0.0f` (a POD `{0,0,0}` is static data, an inline-call initializer
  gives `stfsu`); other units keep the `extern f32 m3r[3]` view through an asm-labelled alias
  declared at the same header position (`extern cMot3Rate m3rObj asm("m3r")`, .bss order = first
  declaration).
- An inline member of the vtable-owning class defined *out of class* in that unit (`inline void
  cPlayer::subCharLiveCheck() {..}` in pl_class.cpp, declared in-class in player.h) is still emitted
  after the destructor in declaration order, and every other unit calls it out of line (cPlayer::move
  `bl subCharLiveCheck__7cPlayer`); an in-class body would be inlined there.
- Static data members mangle as `_7cPlayer.SPEED_WALK_TURN`; sync_symbols demangles them now
  (renamed the pl_class placeholders). Function-local statics with a DECL_UID suffix
  (`pl_move_func_tbl.1272`) are matched by base name in strip_unused (`--gcc`), so an unreferenced
  static table of a STRIP_UNUSED unit survives like in the DOL.
- `pl->x3E0++; if (pl->x3E0 >= 5 && pl->x3E0 <= 14)` compiles to `subi r11,r9,4; cmplwi 9` on the old
  value (combine folds through the increment) with the store issued before the compare (JumpFall).
- Identical case bodies written twice (`case 0xB:` and `case 0xC:` each with the full body, Crouch)
  keep two tree nodes (`cmpwi 0xc; beq; blt`); a shared `case 0xB: case 0xC:` label is a range
  (`subi; cmplwi 1; ble`). Turn180's `switch (x4FB8)` needs `case 0:` as its own arm (same body as
  default) plus `case 5:` grouped with `default:` for the `cmpwi 2` root.
- (player) `pl->pNeck->motL = 0` followed by a global read (`PlFanceFlag`): the load stays below the
  store only through a `void*&` setter (`PSet`), like the `pG` reloads. `hp = pGS->pl_life` (struct
  view) after `pPL = this` keeps `lwz pG` below the scalar store.
- (pl_npc) `cSubChar` is a cEm whose partner fields overlay the player ones (em.h unions at 0x378,
  0x3E0, 0x3E4..0x400, 0x404..0x524 incl. a second `MotionWorkSub subBackMot` at 0x454, and the
  0x5CC/0x7D4.. tail); `subFlags`/`subFlags2` are `cFlag`s: tests written as
  `((cFlag*) &subFlags2)->check(bit)` reproduce the `lhz; mr rX,r0; clrlwi r0,r0,16` copies gcse PRE
  gives a HImode member load (plain `& mask` tests fold adjacent halfword tests into one word compare
  and never show the mask). `&= ~bit` on the u16 flags is `BitOff16` (`rlwinm`, not `andi.`).
- Routine-byte blocks (`xFC..xFF` + a mode word): `SubRoutineSet(pl, fc, fd, fe, ff)` (int inline)
  followed by the other stores gives the original order when the zero pseudo stays live (`RS(1,0,0,0);
  mode = 2;` -> `fc, ff, stw, fd, fe`; `RS(1,0,0,0); dmHit = 0;` -> `324, fc, fd, fe, ff`); brute-force
  the alternatives with a 6-line test file through tools/ngccc.py + `dtk elf disasm` (seconds).
- A `switch` with a hidden `case N: break;` shifts the compare-tree root (moveFootwork needs
  `case 0x34: break;`); `switch ((u32) f())` gives `cmplwi` range tests; a `default:` written first is
  laid out first.
- (pl_npc, Matching) Judge functions by masked *bytes* plus resolved reloc targets, not objdiff's
  percentage: dtk synthesizes `Sym+off` relocs the compiled object lacks, so 30 byte-identical
  functions showed 97-99%, and one "identical" function (jumpAdjust) had its pool words permuted —
  the masked bytes matched while the constants were assigned to the wrong fields (a = {300,300,0},
  not {0,0,300}). Compare pool *values through the reloc targets* before believing a match.
- Constant-pool order idiom, confirmed on 9 functions: a `const f32 name = literal;` local declared
  before the first use creates the pool entry at the declaration with every use folded, no code
  change (moveMove 400 first, getScrActionPoint 400 before -1000, moveFallWait 1300/0.314 first,
  neckCtrl both speeds first, checkBackEm 4e8/2.5e7/2.618, frontCheck 1500, anaSatInfo 360000,
  jumpAdjust). A dead `f32 x = C;` is dropped at tree level and creates nothing; a const declared
  *after* the stores is dropped too. An unreferenced pool word (the static initialiser's 1000) is
  NOT a kept dead entry: mark_constant_pool drops those in the original as well — it is the pool of
  a dead-stripped function emitted right after (an in-class inline of the vtable-owning class,
  `cSubChar::farCheck`, unit in STRIP_UNUSED).
- Unused `static inline` functions parsed at the END of a unit reorder the vtable/static-init output
  (the static init's pool moved before the vtables): put dead pool-only helpers before the function
  whose pool they precede.
- `__static_initialization_and_destruction_0` is emitted before the out-of-class `inline` members
  (setFace/setHand/initCloth/moveCloth) and the `_GLOBAL_.I` thunk; non-inline definitions of the
  same empty virtuals land before it.
- A switch whose adjacent same-target cases are tested one by one (`cmpwi 7 beq; cmpwi 8 beq` instead
  of a range) had one body per case value (duplicated bodies, cross-jumped later): group_case_nodes
  only merges consecutive values that share a label (moveDamage's subHideMode switches).
- A value computed into a local before the member store (`f32 y = expr; eyeDir.y = y; if (eyeDir.z
  == 0) eyeDir.x = y;`) issues the expression's constant loads before the compare's; the member-store
  form (`eyeDir.y = expr; ... = eyeDir.y`) schedules the compare first. `x = x*z + y*(1-z)` written as
  a member function of the static's class loads z before 1.0 (moveFace).
- `SubRoutineSet(this, 0, md, 0, 0)` with `int md = 1;` declared at the top of the case block: the
  `li r6, 1` is shared by both if/else arms and lets jump2 cross-jump their AtariOn tails (control).
- Byte-store order rule (dmgCheck/control): the emitted order is not the source order; brute-force
  the 3-4 statement permutations with a scripted loop (tools: ngccc.py, ~0.3 s per variant).
- cAtariInfo flag stores through the info's address (`cAtariInfo* at = &atari; AtariOn(at, 0x300)`)
  give `addi rX,this,0x2b4; lhz 0x1a(rX)`; a scalar-reference store on `at->flags` keeps a following
  `lwz pG` below it.
- Two `MotionSetCore` calls that share their tail in the target are `void* m; if (..) m = A; else
  m = B; MotionSetCore(pl, .., m, ..)` (arms compute `arc->ofs[n]`, the `add` after the join); a
  ternary index gives `lwzx`.
- `MotionMove` is called with two arguments by the partner code (`MotionMoveF(m, 0) asm("MotionMove")`).
- Frame size = ALIGN16(8 fixed + ALIGN8(vars) + 8 fpmem + ALIGN8(gp+fp saves)): the "unexplained
  8-byte slot before the fpmem slot" (merchant buyupPrice/sellPrice, option) is usually this 16-byte
  rounding, not a local. A frame that is 0x10 bigger with the *address-taken* locals starting 0x10
  later (`addi r28, r1, 0x18` instead of `0x8`) is an unused aggregate local declared before them
  (option `ChapterEnd::move`: `Vec unused;` — addressof slots are assigned after declared aggregates).
- cse_insn's `(set REG0 REG1)` swap: `tbl = &sym; t = tbl;` gets rewritten to `t = &sym; tbl = t`
  (the lo_sum's dest becomes the copy's dest) when t's REGNO_LAST_UID is later than tbl's and the
  previous non-note insn is tbl's set. Any statement between the two (`ofs = 0;`) blocks it, so the
  `addi` result stays `tbl` and the stepping pointer is the `mr` copy; a second copy taken from `t`
  (`t0 = t`) after that is what cse2 turns into the "lhzx base" register (stage subMissionSt1).
- global.c priority truncation ties (`int(10000*log2(refs)*refs/len)`): OpeSetOpenTerm's params x
  (4 refs/216) and z (2 refs/54) both give 370, so the lower pseudo (x) wins f30; the original
  breaks the tie the other way. REG_LIVE_LENGTH here is `recompute_reg_usage` after sched1 (real
  insns only: block notes and store order changes do not count) — one more real insn in x's range is
  needed, not found yet (OPEN).
- loop.c invariant threshold with a call in the loop is `1 + n_non_fixed_regs` = 71: a `lis` (savings
  1, lifetime 1) is hoisted out of an outer loop only while the loop has <= 71 real insns at loop pass
  2. merchant buyupPrice(ItemWork*) hoists ours (65 insns) but not the original's — its outer loop
  had >= 72 pre-combine insns (OPEN: which extra RTL).
- Dump-script note: the build passes no `-G` to cc1plus (cflags have none); a hand-run cc1plus with
  `-G1024` changes small-data references and callee-saved counts. Use `-O2 -mfast-cast -da` only.
- mercenaries MercSysInitRoom (OPEN): the `wk->stage = 0` zero is a reload-materialised pseudo in
  the original (`li r11,0` right before the `stw`, after `lwz pG`), ours a sched1-hoisted `li r0,0`
  (local-alloc'd); and the SndStrReq `lfs f1` pool load is issued last (right before `bl`) although
  its `lis r29` sits at the block top — sched2 in ours hoists it at once. Reference stores (`BitSet`),
  `G_ROOM_ID`, `pGS`, a shared `zero` local, int/local forms of the 0.0f argument all tried.
- Giv final value (loop.c) as a later loop's bound: an `addi r0, base, 0x58` right after an inner
  loop's exit (inside the outer loop's latch) plus `mr rB, r0` in the next loop's preheader is
  loop.c's `final_giv_value` of the inner loop's pointer giv, copied by cse2 into the later loop's
  `&node[2]` bound. record_giv only skips the "replaceable" shortcut when the giv register is
  mentioned *after* the loop (REGNO_LAST_UID beyond loop_end), so the pointer must be ONE
  function-scope variable shared by all the node loops (`EmTreeNode* n; ... n = &node[i];`);
  block-scoped `EmTreeNode* n = &node[i]` per loop gives a fresh `addi rB` instead. The later loop
  also needs the dead `if (i == 2) nx = node; else nx = &node[i + 1];` block: its `i == 2` compare
  makes loop.c place the bound in the preheader (without it `&node[2]` is rematerialised at the
  loop bottom, `where = insn` when threshold < insn_count). emtree/emwep/emshield/emmine R1_Fall.
- Tail-`Normalize` blocks: read the *frame offsets* of the three Vecs from the `lfs` of the mat
  stores, not the names — emshield's tail is emwep's (`Vec b, c, a; Cross(a,b,c); Cross(c,a,b);
  Normalize b, c, a`), not emtree's, and the `#line` numbers follow the real source.
- `u16 flags = p->flags; u32 fl = flags;` (promoted HImode load widened) gives `mr r11, r0`; an
  `int` load copied into a `u16` gives `clrlwi 16` (em_sub EmYarareDisp, still 1 word off: the
  original's copy is not cprop'ed into the later tests while ours propagates one of them).
- A `u8` member passed to two int-parameter inlines (`EmSetDieCk(em->emsetNo)`;
  `EmSetDieOn(em->emsetNo)`) is one QImode load + `mr r6, r9` copy with `clrlwi 24` at each use;
  a `u8 no = em->emsetNo` local is promoted and never masked (em_set EmSetDie).
- Zero-store block order (PenCloth setters): the dying zero store is the *last* zero statement in
  source and is issued first; put that member (`c->x54 = 0` in Em30ClothSet2) after `flags = 0`.
- A distance sum whose result register is the *variable's* callee-saved FPR (not the tied f13 of a
  dying operand) means the variable is assigned in two places: assign the player-distance check to
  the same `d` as the loop's check (emBarred emBarredNearCk).
- OPEN family, likely one compiler-build difference (candidate #8, FPR argument deaths): the
  original ranks the prologue copy `fmr fN, f1` of a float parameter *after* every GPR copy and
  store of the block (emwep setThrow `mr r26,r5; addi w; fmr f30,f1`, emshield setFall
  `mr r31,r4; stw pMotion; fmr f29,f1`, emwep setFall matched only because grav is the last
  parameter), i.e. as if f1 did not die there (weight +1); the same "FP arg register does not die"
  reading explains compiler-build difference 1 (`fmr f1, x` arg moves issued before `li`/`mr`
  int arg moves). No source form changes it; the SatMgrCreateF / atariInitF floats-first aliases
  remain the workaround at call sites (emBarred emBarredEatSet sub[0]/sub[1]).
- Dying-register tie-break not applied by the original (OPEN): emrock SetRock's `stb r30, 0x95`
  (last use of the zero pseudo) stays in source order and `li r30, 0` is re-materialised after the
  next label; emwep emWep_R1_ShotArrow's `mr r3, part` (part dies) is issued *last* of the arg
  moves where ours puts a dying copy first. Both look like a spilled/REG_EQUIV pseudo reloaded
  per label region, but a shared `zero` variable is allocated a register (even a 9th callee-saved).
- Byte-compare tool of record: judge functions by masked words with intra-object branches resolved
  by *target symbol* (NgcAs emits REL14 for every conditional branch and REL24 for local `bl`s; a
  size change in an earlier function shifts every later displacement without a real diff), and
  accept the split object's raw `lis rX, 0x8023` words whose `lfs` lives in another block (dtk
  could not pair them; the linked bytes are identical).
- (esp/espgen water) `lwz r3, pLog` issued BEFORE the string `lis r6` in an error block = the block
  ends in a `return` (jump to the function's return label), not a `goto fail`/fallthrough into an
  else: `if (buf == NULL) { pLog->warn(...); return; }` with the rest un-nested (Espgen42/45
  TransSub), and for an `int` function whose fail paths share one `li r3,0`, the body is
  `if (SetWaterWork(...) != NULL) { ...; return 1; } return 0;` with `pLog->err(...); return 0;`
  in the early check (both `return 0`s cross-jump into the final out-of-line `li r3,0`;
  `goto fail; fail: return 0;` keeps the layout but schedules `lis` first). `nx = 0xB8;` written
  BEFORE `pLog->warn(...)` puts its `li r27, 0xb8` ahead of the call's `li r4/r5` (Espgen4x
  SetFreeWork, both 100%).
- Dead literal stores at 8-byte stride (`stfs f13, 0x8; stfs f0, 0x10; ...` never read, in
  Espgen42/45 Move00): four `Vec` locals whose `.x` and `.z` are assigned literals and never used
  (`Vec d0; d0.x = 1.0f; d0.z = 0.0f; ...` — Vec locals are 0x10-rounded, so x/z land 8 apart and
  .y is skipped); `const f32&` reference temporaries of an empty inline emit nothing.
- `addi r9, r1, N; stb r0, N(r1); psq_l fX, 0(r9), 1, qr2` (own 4-byte slot, not the fpmem one) is
  the inline-asm `PSQ_L_U8(&tmp)` on a function-scope `u8 tmp = noise[i];` (Espgen42/45 Move00;
  the compiler's own u8->f32 goes `stb/psq_l` through the shared fpmem slot).
- `sizeof(T) * (p->nx + 1) * (p->ny + 1)` (constant first, nx before ny) gives the target's
  `lhz ny; lhz nx; ...; slwi/mulli (ny+1); mullw (nx+1), that`: combine folds the constant into
  the SECOND factor and the shifted operand is loaded first; `(p->nx + 1) * 2 * p->ny * 12` for
  the display-list size. `u32 n` sized from `p->nx * p->ny` loads nx first only when written
  `p->ny * p->nx` (Espgen45 Move00 `idx`).
- A `k + 1` used twice and then `k++` (`p->nx + (k + 1)` stores, `k++` at the loop end) gives the
  target's `addi r7, r3, 1 ... mr r3, r7` (cse turns the increment into a copy of the shared
  pseudo); `k++` before the uses increments in place (SetWaterWork dl loops).
- Loop counters that must not be hoisted above the preceding calls (`li r28, 0` right before the
  first store, "Don't let it cross a call after scheduling if it doesn't already cross one") are
  variables that cross NO call: give each loop nest its own counter (`int i2`, `i3`, `i4`) — a
  function-level `i` reused in a loop with `fRand1_1()` crosses the call and its `li` floats to the
  block top. Register order then follows global.c pass 0 (`regs_used_so_far`): the loop-1 `i` gets
  r28 only because the disjoint loop-2 counter took r28 first.
- A static read inside a store loop that the target reloads every iteration (`lfs f0, g45_init_y`
  after `lwz p->pos`) is a reference read `FGet(g45_init_y)` (MEM with neither struct nor scalar
  flag stays below the in-struct `stfs`); `int base = p->ny * (p->nx + 1);` hoists the row offset
  into `mulli r11, r8, 0xc` + a stepping giv.
- OPEN (Espgen42/45 Move00 inner loop): the target keeps `k*4` (r31) and `k*12` (r29) as the only
  reduced givs and computes `c = cur + k*4` with an `add` per iteration (`c[-1]`, `c[1]`,
  `subf r9, r21, r7` for `c[-1-nx]`), while ours strength-reduces `&cur[k]` and both neighbours into
  stepping pointers (loop dump: giv 135 `cur + k*4` combined with the `cur[k]`/`c[±1]` DEST_ADDR
  givs and reduced). Pointer/byte-offset/`u32`/label-between forms all reduce; the target's
  `c` is either ignored (`lifetime * threshold * benefit < insn_count`) or not a giv
  (k4 `cant_derive`). Both Move00s are otherwise structurally aligned (registers p=r28 etc.).
- OPEN (esp0a Trans/SetFreeWork, esp0e Trans; candidate compiler-build difference): the implicit
  copy-assignment of a polymorphic object (`*base = *esp`, vptr at 0xF4 saved to a frame temp and
  restored) loads the saved vptr (`lwz r0, 0x8(r1)`) only AFTER the leftover block-copy stores
  in the original, so the copy temp is r0; ours hoists the `mem/f` (scalar) frame temp above the
  in-struct `stw` leftovers, the restore pseudo takes r0 across them and the copy temp gets
  r9/r10/r11 (t4-style `struct A { int x[61]; virtual void f(); }; *a = *b;` reproduces it in
  isolation). Only these two units have the pattern.
- `esp43`'s stray `.rodata` zero word is a dead-stripped static with one `x != 0.0f` compare
  (`Esp43_SetPos`, STRIP_UNUSED); `esp_app`'s 4-byte `.rodata` tail is the 8-alignment pad before
  `esp_efm` (our .rodata is 4-aligned, the linker re-creates the gap).

## CRI middleware (`lib/adx_*`, `lib/sfd_*`, ... — CodeWarrior 2.4.7)

The CRI ADX/Sofdec libraries, the GCCI/MFCI CVFS interfaces and CRI's `UTY_*` helpers were prebuilt by
CRI with **Metrowerks CodeWarrior 2.4.7** (the `.rodata` build strings say `Append: MW2407
GC20Apr2004Patch1` = compiler 2.4.7 on the Apr 2004 patch 1 SDK; `GC/2.0`, `2.5`, `2.6`, `2.7` are all
2.4.7 and produce identical code on every unit tried; `GC/2.0` is configured). Flags (`cflags_mw_cri` in
configure.py, `CRI_LIBS` lists the units): `-O4,p -inline auto -sdata 0 -sdata2 0 -str readonly
-use_lmw_stmw on -char signed`, no small data at all. Evidence: `stwu r1,-0x10; mflr; stw r0,0x14`
prologue (1.2.5 emits `mflr; stw r0,4(r1); stwu`), `__div2i`/`__mod2i` runtime calls, `mr. r31,r3`,
`stmw/lmw` saves, every 4-byte global addressed `lis/addi`, float constants and strings in `.rodata`.
The ADX group was built Oct 8 2004, Sofdec Sep 22 2004. Headers: `src/lib/cri/` (`cri_xpt.h` types,
`sj.h` stream-joint interface). Workflow is the SDK one (`strip_unused.py --unit` post-build, so dead
functions must be written when their pools/statics survive). Bio4.sym `local` scopes are wrong for
many CRI functions (`SFMEM_ExecServer`, `MPVM2V_Finish`, ...): a `static` that another split object
imports makes ngcld exit 99 silently — bisect by swapping compiled objects for split ones in
`build/G4BE08/main.elf.rsp`.

MWCC idioms seen so far (2.4.7, -O4,p):
- Uninitialised file-scope data (globals and statics) is emitted in order of **first reference** in
  the code, not declaration; `= 0`-initialised scalars still go to `.bss` but at their declaration.
  An original `.bss` order that no live function produces means a dead function referenced them
  first (adx_bahx `ADXB_EntryAhxFunc`, adx_insh `ADXT_GetDmyBuf`).
- Zero-initialised aggregates (`= {0}`) go to `.data`; a zero scalar in `.data` needs
  `#pragma explicit_zero_data on` (gcci_sub). An unreferenced pointer to a dead-stripped function
  is left as a zero word in `.data` (cft_common).
- Statics of one section are addressed through the section pool symbol (`...bss.0`,
  `lis/addi` once, then `lwz off(rBase)`); a global gets `lis/lwz sym@l`. A `volatile` scalar is
  re-read after every store (`lwz` twice); a bare `x;` statement of a volatile is a real load.
- The version string is kept alive by `static const Char8 *const volatile xxx_build = "..."` read
  as a bare statement at the top of the init function (dead `lwz r0,0x3c(rPool)`).
- Float constant pools are per function, emitted in function order; inside one function the order is
  not use order (split constants over dead functions to reproduce a pool).
- `if (x >= 0) return A; return A+1;` folds into `srwi/subi/add` arithmetic; the order of the `-1`
  and the `lis` depends on which value the first branch returns (muldiv).
- `Sint32 sz = sizeof(Sint64); if (sz < 8) for (;;) {}` is NOT folded (`li r0,8; cmpwi r0,8; bge`)
  — CRI's compile-time check idiom (cmptime).
- `ret = 1; else ret = 0; return ret;` keeps the branches; `if (c) return 1; return 0;` becomes
  `neg/subfic` flag arithmetic.
- Loops: `cnt = n + 1; while (--cnt) {...}` gives `addi; b check; body; check: subic.; bne` (the
  UTY_Memcpy/MemsetDword loops); `while (cnt--)` / `for` become `mtctr/bdnz` and are unrolled 8x
  at -O4. `for (i...) { p = &arr[i]; ... }` gives the direct `addi r31,r3,arr@l` induction pointer;
  `p = arr; for (...; p++)` copies it through r0 (`addi r0; mr r31,r0`).
- `if (a == NULL || n <= 0) return;` gives `beq end; cmpwi; bgt body; b end`; a separate
  `if (n > 0)` gives `ble end`.
- 64-bit compares: `x <= y` is `xoris/subfc/subfe/subfe/neg.`; `min = ts->min; if (t < min) min = t;
  ts->min = min;` (if with a local) and `ts->max = (t > ts->max) ? t : ts->max` (ternary) give the
  two branch shapes of sfd_tmr.
- Locals: later declarations get lower frame offsets (sfx_set `inf`/`out`, adx_insh). Callee-saved
  registers go r31 downward in declaration/parameter order; a pointer parameter copied to a local
  (`Uint8 *p = dat`) is allocated after the other parameters (sud_lib).
- 8-byte-aligned structs are copied with `lfd/stfd` pairs, 4-byte-aligned ones with `lwz/stw`
  (mps_get).
- Error returns: `b end` straight after a `bl SFLIB_SetErr` with no `li r3` = `return SFLIB_SetErr(..)`
  (the callee's result is returned); `beqlr` after a NULL test = `return p;` not `return NULL;`.
- A `switch` with one live case and `beq case; bge default; b default` has a second case label that
  shares the default body (`case 2: default:`); `beq case; b default` is a lone case.
- A `mtctr n; loop; ... li r3,0` search with an early exit to the code after it is an inlined
  `static` helper `for (...) { if (p->used == 0) return p; p++; } return NULL;`.
- 32-byte struct copies are unrolled `lwz/stw` pairs, 128-byte ones become a `mtctr 16` loop of
  `lwz/lwzu/stw/stwu` with both pointers pre-decremented by 4 (`subi r5,rDst,4`).
- Division by a constant: `mulhw M; add; srawi s` with a "negative" magic means the unsigned magic
  M gives d = 2^(32+s)/M (mpv_get: 0x91A2B3C5, s=10 → /1800).
- `crclr cr1eq` before a `bl` = the callee is variadic (`MWSFSVM_Error(const Char8 *fmt, ...)`).
- Register order of callee-saved variables is neither declaration nor first-use order (mpv_frm
  `MPV_SkipFrmSj`: original mpv=r31, code=r30, sj=r29, ours r29/r31/r30; sfd_uo `SFUO_Create`
  reuses the `uo` register as the stepping induction pointer; sfx_alp `SFXA_Create` constant
  registers) — OPEN, all statement permutations of SFXA_Create's init block were brute-forced.
- OPEN (mpv_cmc `MPVCMC_InitMcOiRt/InitObj`): the original keeps `addi r5,r3,0x124` as a separate
  base for six `stw off(r5)` stores into a member array while every source form tried (pointer local,
  loops, casts, volatile, inline helper) folds the offsets into `r3`.

### SN libsn / ProDG runtime units (`lib/dummy`, `tealeaf`, `FSasync`, `sndvd`, `fileserver`, `crt0`, ...)

`dummy` (stdio syscall stubs), `sndvd` and `FSasync` are GCC 2.95 -O2 like the game; `tealeaf`
(`__cvt_fp2unsigned`, the MWCC-ABI `__va_arg`, `__div2i`-style aliases that `b` to libgcc) is
hand-written assembly (its libsn.a member carries a `tea151.tmp` FILE symbol like proview/ppcdown,
the C members carry `<name>.c`; `addi r7,r3,0`, `subi/nor` for `~(n-1)`, two zero registers) and is
an `ASM_UNITS` entry. The v393 `libsn.a` in re4-orig/prodg has every libsn member with full symbol
names (sndvd's `NotDvdDsi` label, its `g_hDVD` etc.) (`stwu -8; mflr; stw r0,0xc`, `stmw`,
`first.183` statics, r9/r11 temporaries) but with **no small data** and **no common symbols**
(`LIBSN_UNITS` in configure.py: `cflags_game + -G 0 -fno-common`, `strip_unused.py --gcc`; without
`-fno-common` FSasync's uninitialised globals become COMMON and leave the unit's `.bss`). `proview`,
`ppcdown`, `fileserver` (`addi r31,r4,0` copies, `lis rX,sym@h; ori rX,rX,sym@l`), `eabi`
(`_savefpr_14/_restfpr_14`) and `__start` (`.init`) are hand-written assembly and are built from
**`src/lib/<name>.s`** (configure.py `ASM_UNITS`: the split name stays `lib/<name>.c`, the Object's
`source` is the `.s`, so tools/project.py's `asm_build` uses the template's `as` rule =
`build/binutils/powerpc-eabi-as -mgekko --strip-local-absolute -I include -I build/G4BE08/include
--defsym BUILD_VERSION=0` + `dtk elf fixup`; `macros.inc` provides `.fn/.endfn/.obj`). The sources
are the dtk disassembly with the address comments stripped, `bl`/`b` to global function starts made
symbolic (relocations resolve to the same displacement), `_stack_addr/_SDA_BASE_/_SDA2_BASE_` for the
`.init` register setup, and one fix: dtk prints `ori r0,r0,imm` as `nop`, so proview's
`lis r0,sym@h; nop` was really `ori r0,r0,sym@l` (the DOL check catches it: 2 bytes). `__start` is an
absolute symbol of ldscript.ld, the `.init` code carries the local label `__start_entry`. Branches
into data (`proviewtty`) or into the middle of other units' functions stay raw displacements with a
`# -> 0x8... (sym+off)` comment; objdiff shows ARG_MISMATCH on assembler-resolved local branches, the
linked bytes are identical. `crt0` is only the data half of that assembly (two 32-byte message
buffers, the libsn version words, `LinkFiddle = {__mod2i, 0}`); `crtbegin` is `.ctor/.dtor` `-1`
sentinels; `builtin-delete` is SN's libstdc++ `operator new/delete` warning unit (C++, everything
stripped but the `bad_alloc` type-info name and the four warning strings). The split object's `.data`
alignment (dtk reports 2**3) is what the DOL layout needs: a GCC object with a 4-aligned `.data`
shifts every following `.data` unit by 4 (dummy: `asm(".section .data\n\t.balign 8\n\t.section .text")`).

FSasync (matched) idioms, GCC 2.95 -O2 -G 0:
- Every reload of a global after a store to it (`stw; lwz; cmpwi`) and re-reads inside one expression
  = `volatile` globals (the EXI2 transfer state). A volatile store never moves above a volatile load
  (`cb = g_FSCBFunc; g_nRWasyncPhase = 0; if (cb)` keeps `lwz` before `stw`); a plain load can.
- `g_nBlockCnt--; while (g_nBlockCnt != -1) {...}` gives separate `lis sym@ha` pseudos per block;
  `for (;;) { g_nBlockCnt--; if (g_nBlockCnt == -1) break; ... }` (exit test duplicated by jump.c)
  shares one `lis` between the pre-loop copy and the body and stops loop.c hoisting it out of the
  outer loop — the target's `lis r30,g_nBlockCnt@ha` at the top of each outer iteration.
- `li r0,0x10; slwi r0,r0,8` (an unfolded constant) is a single-use local (`u16 hlen = 0x10;`) set
  before a loop and used after it: cse cannot fold across the loop and update_equiv_regs moves the
  `li` next to the use.
- A struct whose address is taken before a loop (`struct FSResult *res = &g_FsResult;`) keeps
  `lis/addi` in a callee-saved register across the calls; `&g_FsResult` at each use rematerialises
  the `addi`. A DMA target declared `__attribute__((aligned(32)))` gives the 4-byte `.bss` gap before
  it and the 0x20 rounding after it.
- Raw hardware addresses (`*(volatile u32 *)0xCC003000`) are `lis/ori`; a clear-byte loop
  `for (i...) *p++ = 0` is `mtctr; stb; addi; bdnz` (memset would be a libcall).

sndvd (matched) idioms: the DABR write goes through an `"m"` asm operand (`asm volatile("lwz 3,%0; mtspr
1013,3; isync" :: "m"(dabr))`: stack slot + hard r3); the DSI exception entry is one top-level `asm`
block with `.type/.size` (its trailing `li r3..r6,0; bl DSIHandler` is part of the asm); the DI
register copy loop reads `*(volatile u32 *)((0x0C006000 + i * 4) | 0xC0000000)` (physical address
OR'd with the uncached base inside the loop -> `oris` per iteration); `asm(".long 1")` after
`OSReport` is the debugger trap word; `switch (cmd)` with `int cmd` for `cmpw`; store order of six
globals found by brute force (`perm.py` over the statement order); `asm volatile("")` after the
default case's `ForceDvdDeIrq()` blocks the single-insn tail cross-jump (COMPILER-DIFF #6). OPEN:
`ctx` (7 refs / 153 insns) ranks below `pos` (3 refs / 29 insns) in global.c's allocno priority and
gets r28 where the original has r29; a dead `asm volatile("" :: "r"(ctx))` supplies the 8th
reference.

SDK/CRI MWCC register-allocation levers found on reverb_std, svm and ax_rna (MWCC 1.2.5n / 2.4.7):
- reverb_std `ReverbSTDCreate`: `max_length << 2` (not `* 4`) in the inlined `DLcreate` decides whether
  `rv` gets r31 or r23 (same code otherwise); the identical `* 4` form matched in reverb_hi.
- Callee-saved registers of a function's own locals: the *last* declared gets the highest register
  (svm_exec_svr: `p; i; ret` -> ret r28, i r27, p r26); strength-reduced loop pointers normally take
  the registers above the locals (SetOutVol `ptr r31, v r30, i r29`), but a loop living in an
  *inlined static helper* puts the helper's locals above the pointers (AXRNA_ExecHndl: n r30, i r29,
  pointers r28..r26) and the helper's aggregate locals get frame slots in declaration order upward.
- A loop-invariant expression written from a *block-local* copy (`Uint32 f = adj;` inside the `if`)
  is not hoisted out of the loop; written from the function-level variable it is hoisted.
- Two independent `srawi ..,16` of the same value = two locals initialised from the same field
  (`loop = rna->buf[i]; cur = rna->buf[i];`), the loads are CSE'd, the shifts are not.
- `hist[pos++] = v` (post-increment in the index) vs `hist[pos] = v; pos++` swaps the temporaries of
  `pos+1` and `pos*4`; a separate `nbyte = diff * 2` local before a loop moves the loop counter above
  the hoisted product.
- `if ((p->x = f()) == NULL)` tests r3 straight after the call (`cmplwi r3,0; stw r3`); a separate
  `if (p->x == NULL)` reloads. `p->y = f(); if (p->y == NULL)` was used for the SJRBF_Create case.
- Volatile file-scope counters (`svm_lock_level--; if (svm_lock_level == 0)`) reload after the
  store; the error-callback pair `{func, obj}` is a plain struct: `lwz r12,off(rBase)` for `.func`
  but `addi r3,rBase,off; lwz r3,4(r3)` for `.obj` (member at offset 4 of a pooled static).
- An unrolled `stw 0(r6) .. 0x14(r6)` clear through the array's address = `p = arr; for (...) *p++ = 0;`;
  with `arr[i] = 0` the first store folds into the pool base.
- `.bss` first-reference order and `@N` string order need the dead functions written (svm:
  `svm_itoa` with `static Char8 buf[32]`, `SVM_SetCbWaitVsync`, `SVM_SetCbTestAndSet`, `SVM_SetCbLock`,
  `SVM_SetCbGotoSvrBorder`, `SVM_GetNumCbSvr`, `SVM_ExecSvrFuncId`, `SVM_ItoA2`; ax_rna: `AXRNA_DbgDump`
  with a local `const Char8 *sw_str[2] = {"OFF", "ON "}` -> the anonymous `@N` pointer table in
  `.rodata`, and the public getters/setters that are only ever inlined: `AXRNA_GetPlaySw/GetTransSw/
  SetSrcType`). `= 0`-initialised scalars keep declaration order.
- MWCC inlines *public* (non-static) functions defined earlier in the file at -O4 (`AXRNA_SetOutPan`,
  `AXRNA_SetSfreq`, `AXRNA_Destroy` appear inline in `AXRNA_Create`/`AXRNA_Finish` with their
  `rna == NULL` checks kept); `SVM_CallErr1` inlined everywhere but emitted after its users =
  static helper `svm_call_err1` + public wrapper.

- Zero-copy chains `li rA,0; mr rB,rA; mr rC,rA` come from *inlined* code: a zero init inside an
  inlined static helper (or its locals initialised at declaration) copies a zero the caller already
  holds; two zero inits in one non-inlined function give separate `li`.
- A search loop whose index is in r3 and pointer in r4 is an inlined `static Sint32 search(void)`
  returning the index; the direct `for` gives pointer r3 / index r4.
- MWCC never hoists a load above an earlier store to memory, whatever the types: a `lwz` between two
  stores means the source read it into a local at that point.
- Callee-saved registers go r31 downward in *first definition* order; a value defined in an inlined
  helper is allocated after the caller's live locals.
- `-O4` unrolling: `for (i = 0; i < n; i++)` with `i` used in the body -> `subi 8/addi 7/srwi 3` form +
  remainder compare; `while (n-- > 0)` -> `srwi. n,3; mtctr; ...; andi. n,7`.
- Float constant pool entries are emitted before the function's strings; a stray 4-byte zero word in
  `.rodata` between strings is a `0.0f` in a dead function.

- `-fp_contract on` is required in `cflags_mw_cri` (`-fp fmadd` alone does not contract in 2.4.7).
- Callee-saved order = declaration order, first declared -> highest register, after compiler
  temporaries (strength-reduced pointers) which take r31/r30 first.
- `long`/`Sint32` loop counters keep the entry guard and reload constants; `int` counters get it
  folded. Loops <= 32 fully unroll; 64 and 192 don't (a 192-store clear is six macro loops of 32).
- Functions emitted after their inlined uses are separate static helpers at the top plus a public
  wrapper later; a standalone `if (c) return -1; return 0;` becomes `subfic/nor/srawi`, the inlined
  copy keeps branches.
- `.bss` first-reference order includes stripped functions and static helper bodies (dead getters
  are needed to place variables).
- OPEN (MWCC): anonymous float-literal pooling — our 2.4.7 pools >=3 literals of a function through a
  `...rodata.0` base; the original never pools literal-only functions but does pool literals together
  with strings. No flag reproduces it (all GC builds and -O levels tested).

- Right operand of a commutative `|`/`+` is evaluated first (`a | b` -> `b` computed, `a` inserted
  with `rlwimi`; `f(0) + f(1)` calls `f(1)` first).
- `x = LE32(p); x = SWAP32(x);` as two statements gives the `mr` copy before the last `rlwimi`;
  one expression gives no copy. A masked byte-swap stored through a `Uint16*` becomes `sthbrx`,
  the unmasked form stays `srawi/rlwimi`.
- `(Uint8)inbuf[i] << 8` with `Sint8 *inbuf` gives `clrlslwi 24,8`; `Uint8 *` gives plain `slwi`.
- `return f() != 1;` -> `subfic/subi/or/srwi 31`; `return f() == 1` -> `cntlzw/srwi 5`.
- Strings of a local static defined inside an inlined accessor at the top of the file come first in
  `.rodata` and reload the pointer per iteration.
- `#pragma dont_inline on/off` around a static definition stops auto-inlining of it (OPEN: original
  did not inline `mwsfd_ExecSvrHndl`/`MWSFSVR_DecodeServer` while inlining smaller helpers; no
  `-inline` level reproduces it).
- Stack slot order: first-declared aggregate local gets the highest frame offset.

- `-inline auto,deferred` (via `CRI_CFLAG_OVERRIDES`) allows inlining of functions defined later and
  emits functions in reverse source order; units whose accessors inline a later helper (mwsfdset,
  adx_fs) are written in reverse order with the override. `.rodata` string order = codegen order.
- `#pragma dont_inline on/off` around the *caller* stops a big static from being inlined while it
  keeps its own body.
- Same-lifetime temporaries take volatile registers in declaration order (first declared -> lowest);
  brute-forcing declaration permutations is cheap and fixed several sfd_mpvf functions.
- `p = base; p += n; p -= 8;` as three statements keeps `add; subi`; one expression reassociates.
- A pointer local used with both a constant and a variable index materialises a base register with the
  constant use folded into the parent pointer.
- OPEN (MWCC): member-address kept in a callee-saved reg across a call with one use; pooled strings in
  reverse use order; dead `b end` after an empty `case N: break;`; static-function literal placement;
  callee-saved order of parameters not by declaration/lifetime/use count.

- Search loops with an early exit written as `beq next; b found` (a redundant unconditional after the
  last `&&` test) plus `li rX,0` on the fall-through are an inlined `static` helper with
  `for (...) { if (A && B) return id; } return 0;` (sfd_hds `sfhds_SearchStmId`); the direct loop gives
  `bne found`.
- `x = (p[0] << 8) | p[1]; x <<= 8; x |= p[2]; x <<= 8; x |= p[3];` gives `rlwimi` for the first pair
  and `slwi/or` for the rest; one expression or `x = (x << 8) | p[n]` chains give all-`rlwimi`;
  `<<=`/`|=` from the start give all-`slwi/or`.
- A pointer derived in two steps (`SFHDS_FHD *fhd = &sfd->fhd; SFHDS_VID *vid = &fhd->vid;`) keeps
  `addi rX,rBase,ofs` as a live base register; `&sfd->fhd.vid` in one step is folded into the loads.
- Ternary arm order: `f(&tmp) == 0 ? -1 : tmp` gives `bne ok; li -1; b; ok: lwz`; `f(&tmp) ? tmp : -1`
  gives `beq`. `(A && B) ? C : 0` becomes branchless `neg/or/srawi/and`; an if/else into a local keeps
  the branches.
- `if (a <= 0 || p == NULL) return;` gives `bne body; b end`; `else if (a > 0 && p != NULL) {body}`
  gives a plain `beq end`.
- Integer `add` operand order follows the source (`add rD, rLeft, rRight`); pointer arithmetic is
  canonicalised (pointer first) and reassociated (`(buf + ofs) + n` -> `buf + (ofs + n)`). A target
  `add r3,rOfs,rBuf; add r3,rN,r3` is `(Uint32)ofs + (Uint32)buf` then `n + that` as two statements.
- A struct field read once into a local and used across blocks vs. re-read `sj->bsize` at each use
  changes volatile-register numbering (CSE keeps the reload in the same register anyway); when the
  target's temporaries look "reversed", drop the local.
- A `Sint32` function result reused as the return value (`nbyte = 0; ...; return nbyte;`) keeps the
  parameter's register for the result (sj_rbf `SJRBF_IsGetChunk`).
- `#pragma dont_inline on/off` around a public function stops it being inlined into later functions
  (sfd_hds `SFHDS_ProcessHdr`/`sfhds_SetHdrRaw` are called, `SFHDS_IsSfdHeader` is inlined) but
  also stops static helpers being inlined *into* it: use a macro for those.
- MWCC 2.4.7 has no `__dcbi/__dcbz_l/__mfspr` intrinsics (they become calls); `asm { dcbi p, i }`
  with `register` operands in a `for (i = 0; i < N; i += 0x20)` loop is unrolled 6x (156 = 6*26
  iterations for 0x1380 bytes) exactly like the original; `asm { mfspr r0, 920 ; stw r0, hid2 }` gives
  the target's `mfspr/stw/lwz` through a stack slot (mpv_lib).
- A parameter needed in an `asm` block as `register` in the *prologue-copied* form: `register MPV p =
  mpv;` as the first local (all uses through `p`) keeps `lis` of an inlined store above the `mr.`
  copy; `register` on the parameter itself does not (mpv_lib `MPV_Destroy`).
- Counted handle loops (`mtctr nhn ... bdnz`) need the count and base copied to locals before the loop
  (`n = wk->nhn; p = wk->hn;`); indexing the struct members directly reloads them per iteration. A
  second such loop in the same function used fresh block-scoped locals (volatile registers) rather
  than the callee-saved ones (mpv_lib `MPV_Init`).
- A callee that ignores its arguments still receives them: `MPVM2V_SetCond(mpv, id, val)` keeps r3
  live so the `cond` pointer takes r6, not r3.
- Split objects' `.bss` (and `.rodata`) are padded to their alignment by dtk (sj_mem 0x484 vs 0x488,
  mpv_lib 0xAE vs 0xB0): a 4-byte/2-byte trailing `lbl_` gap needs no dummy variable.
- OPEN (sfd_hds `sfhds_DoProcessHdr`/`SFHDS_SetHdr`): the original allocates callee-saved registers as
  params (reverse order, r31 down) then locals (`fhd r31, sfh r30, ver r29`; `result r30, len r29,
  p r28, sfd r27`); ours gives locals first (`ver r31, fhd r30, sfh r29`) or params last. Inlined
  helper values, block scoping, `register`, statement order and a dozen structural variants tried.

- Buffer-table access `sfd->buf[n].field` folds the table base into the displacement; to keep a pointer
  across calls with the folded form use a shifted view type (`struct { Uint8 pad[0x1308]; SFBUF_WORK w; }`).
- A 64-bit result assembled by statements (`hi <<= 32; hi |= pos; return hi;`) frees the argument
  register for the intermediate; the single expression takes a fresh register.
- `static Bool f() { if (a == b) return 1; return 0; }` inlined keeps `subf/cntlzw/srwi.`;
  `return a == b;` inlined folds into `cmplw/bne`.
- MWCC addresses all globals it defines in one .bss via one base (`sym@ha` + offsets).
- A local `Char8 hdr[] = "..."` is a `mtctr` word-copy loop at its declaration point.
- MPEG field extraction: explicit masks `(b >> 4) & 0xF` give `extrwi`/`rlwimi`; unmasked gives `srawi`.
- `for (;;) { if (f()) goto found; ...; if (i >= 3) break; i++; } goto done; found: ...; done:`
  reproduces the found block after the loop with both exits jumping past it.
- OPEN (MWCC, blocking ~60 units): callee-saved/volatile register priority is a computed ranking, not
  declaration or first-use order; brute force of declaration/statement/scope/`register`/types does
  not move it. Also OPEN: `&wk->u.ring` kept in a callee-saved reg; ternary/if-else diamond sunk to its
  use; `adr[8]` kept on the stack; `bne body; b end` after `mr.`.

### MWCC compiler-build differences (sweep result, do not brute-force further)
A full sweep (every GC/1.0..3.0a5.2 and Wii mwcceppc build, every documented and hidden 2.4.7 option
and pragma singly and in 66 pairs, against 7 near-miss functions with identical instruction streams,
plus a 55-function regression set) found NO configuration that moves any of them; all 2.4.x builds
(GC/1.3.2 .. 2.7) emit byte-identical `.text`, and every deviation from `cflags_mw_cri` regresses matched
units. `cflags_mw_cri`/`MWCC_CRI_VERSION` are the unique optimum. Treat these as compiler-build
differences (the original used a 2.4.7 build we do not have) and accept 88-99.9% on the affected
functions instead of more source permutations:
- M1 register ranking: callee-saved order (parameters reverse-order above locals) and the volatile
  temporary preference (e.g. `SFBUF_RingGetRead`: target gives the zero constant the freed r5 and the
  `mulli` result r0; ours zero=r0 and reuses the dying operand in place).
- M2 float-literal pooling: ours pools whenever a function references >=3 distinct `.rodata` objects;
  the original applies that rule to strings but not consistently to float literals.
- M3 auto-inlining decisions for mid-size helpers (mwsfdsvr).
Write the remaining CRI units for source completeness; flag only what matches.
- M4 byte-swap store: our 2.4.7 folds any dead `store(bswap32(x))` into `stwbrx` regardless of
  spelling (only `nopeephole` stops it, which breaks `rlwimi` merging); the Sofdec originals keep
  `rlwinm/rlwimi x3/stw` (ADX originals do use `stwbrx`). Accept 16 bytes/function.
- M5 shift forwarding (Sofdec, mpvabdec): our 2.4.7 forwards a shift definition of a local into
  every rlwinm-foldable use of the same basic block even when the variable is materialised anyway:
  `code <<= 1; n = (code >> 24) & 0xFF` -> `extrwi n,code_old,8,1` + `slwi code,code_old,1`, and
  `x = (Uint32)code >> 22; tbl[x >> 1]; ... x & 1` -> two `rlwinm` from `code` plus `srwi x`. The
  original keeps the materialised register: `slwi r0,r0,1; srwi. r11,r0,24` and `srwi x; clrrwi;
  clrlwi`. No `-O` level, `-opt` keyword or `#pragma` (opt_propagation, peephole, opt_common_subs,
  optimization_level...) reproduces it; ours stops forwarding only when the first use in the block
  is not foldable (a compare/store) or the definition is conditional (`c ? x << 1 : x`). Verified
  by inserting a dummy compare after the shift: the register assignment of the whole prologue then
  matches, so the register residue around such sites is M5-caused, not M1. Costs 1-2 instructions
  and a register renumbering per site (mpvabdec: 6 sites x 3 functions).

- Inlined helper locals are laid out first-declared -> lowest frame offset (reverse of a function's
  own locals); each inlined call gets a block below the previous one.
- `static inline` forces inlining of a helper too large for `-inline auto` (a source-level fix for
  what looks like M3).
- A variable with two definitions gets `mr r0,r3; ...; mr rX,r0` for a call result; single-definition
  variables get a direct `mr`. `y = x` between two live variables stays `mr`; a fresh one folds to `li`.
- Integer add chains reassociate (first addend added last); separate statements keep source order.
- `const T *` parameters let MWCC CSE loads across stores through another pointer; a reloading
  original means non-const parameters.
- Volatile FPRs are assigned in declaration order (first declared -> lowest).
- `lbz` without `extsb` before `cmpwi K` on a `Sint8` field is a single-use `== K` compare.

- `-inline auto,deferred`: `.text` reverse source order, `.bss` reverse declaration order, `.rodata`
  initialised objects in declaration order, float pools in codegen order.
- Table-fill loops: n <= 8 unrolled with no guard; 9..32 unrolled behind a `li 0; cmpwi n; bge` guard;
  > 32 as 32-store `mtctr` loops; a body containing a call-free inner fill loop is not unrolled.
- Constants substituted by the unroller are not refolded (`li r0,4; ori r0,r0,0x700`).
- FP locals get f31 downwards in declaration order; the loop counter declared last takes the lowest
  callee-saved GPR.
- Address-taken `sscanf` outputs are separate scalars with stack slots in declaration order top-down.
- Paired-single inline asm needs the scheduler ON to reproduce swapped adjacent pairs; non-PS asm
  bodies need `#pragma scheduling off`. Inline asm cannot use compiler pool constants: declare
  `static const Float32 x` and use `x@ha`/`x@l`; GQR operands must be numeric.

mpvabdec (`MPVABDEC_NintraBlock/IntraBlock/IntraBlockDc11`, 99.3/99.6/99.6%, residue M5 + M1;
the three 0x400 `.data` tables are 256-entry `switch` jump tables, one per function):
- A dense `switch` (256 cases on `(code >> 24) & 0xFF`) is a `.data` jump table (`@N`, one
  `.rel` per value) with `srwi 24; cmplwi 0xff; bgt after_switch; slwi 2; lwzx; mtctr; bctr`; a
  sparse one (`0, 1, 2-3, 4-7, default`) is a compare tree. Case bodies are emitted in source order
  (the original: coefficient cases 0xFF down to 0x00, then the EOB-terminated cases 0xFE down,
  `default:` first in the sparse switch); `continue` cases branch to the loop head, `break` cases and
  the out-of-range test fall to the code after the loop.
- Struct fields are reloaded after every store (`prm->idx = *++zz; ... prm->iqm[prm->idx]` gives
  `stw; lwz`); several reads in one expression share the reload. `prm->idx = prm->idx0 = v` stores
  `idx0` first.
- `p[1]` then `p += 2` in one block gives `lbz 1(p); lbzu 2(p)` (the increment is deferred into the
  next load); when `p` is dead afterwards the second load is a plain `lbz`.
- `(Float32)i` is the `xoris 0x8000; stw; lfd; fsubs` double trick with the 0x43300000 word stored
  per conversion; two conversions in one block use two stack slots with both high-word stores
  hoisted to the top.
- `Sint32 code` with `(code >> n) & mask` gives the `rlwinm` forms (`srwi`/`extrwi`) and
  `(code << 8) < 0` gives `slwi.; bge`; `(Uint32)code >> n` where no mask is wanted (`srwi x` kept
  as a variable). A `Uint32` look-ahead folds everything into `rlwinm` chains.
- `(prm->level << 1) * prm->qscale` evaluates `qscale` into the first temporary (target
  `lwz r11 qscale; slwi r12 level; mullw r11, r12, r11`); `prm->level * 2` evaluates `level` first.
- `zz = tbl + run` as one expression computes into a temporary and copies (`add r5; lbz; mr r6,r5`);
  `zz = tbl; zz += run;` defines `zz` directly. The operand order of the loop's `add` for
  `zz += prm->run` is NOT spelling dependent (`zz += run`, `zz = run + zz`, `&zz[run]` are
  identical); a pre-loop `zz = tbl; zz += prm->run;` made the loop adds `add zz, run, zz` (target),
  functions without it keep `add zz, zz, run` (OPEN, 5 sites in IntraBlock/Dc11).
- A block clear through a pointer field (`prm->dst[i] = 0.0`) reloads the pointer per store (the
  `Float64` store aliases the field); copy it to a local first.
- Volatile registers of the loop-invariant locals follow declaration order (first -> lowest) but
  a value that is a copy of another variable (`code = bbuf`) is ranked last whatever its position
  (Dc11: target `code r6 ... bbuf r10`, ours `bbuf r6 ... code r10`, M1). The three functions
  needed three declaration orders (Nintra `code, zz, bbuf, nbuf, bitpos, ptr`; Intra
  `bbuf, nbuf, bitpos, ptr, zz, code`; Dc11 `bbuf, nbuf, bitpos, ptr, code, zz`).
- Layouts: `MPV_BLKPRM` 0x00 run, 0x04 level, 0x08 sign, 0x0C len, 0x10 idx0, 0x14 idx (the
  return value: idx, negated when more than one coefficient was stored); `mpv->scale_tbl` is
  `Float32[64]`, `mpv->bitmsk_tbl` is read as `Sint16` (`lhax`), `prm->dctbl` as `Uint8`
  (`(size << 4) | len`); `rl_8` entries are `(len << 16) | (level << 8) | run`, the 11..17-bit
  tables `(level << 8) | run` halfwords (mpv_vlc.c's `RL(len, a, b)` arguments are really
  `(len, level, run)`); D pictures (`picatr.pic_type == 4`) skip the AC loop in IntraBlock only.

## REL modules

The game loads its rooms, enemies, weapons and debug tools as Nintendo REL overlays. `ninja` rebuilds the
110 configured RELs byte-identical next to the DOL (`build/G4BE08/<mod>/<mod>.rel`, all covered by the
`build/G4BE08/ok` SHA-1 check: `111 files OK`); a unit you match replaces one split object in one REL,
like the DOL.

### Facts

- Modules: the 20 loose `files/Rel/*.rel` of the disc (Sscrn, Tools, st1_0..st4_0, t_camera, t_emlist,
  t_esp, t_event, t_id, t_light, t_movie, t_sce) and the 90 distinct RELs inside the `files/em/*.drs`
  archives (43 enemies em10..em3e, 8 players pl02..pl14, 39 weapons wep00..wep47). 117 of the 128
  archives carry a REL; 27 of those are byte-identical copies of another archive's REL (costume
  variants em46/56/66 = em16, em4c/5c/6c = em1c, em4d/5d/6d = em1d, em4f/5f/6f = em1f, em50/60/70 = em20,
  pl0b/pl0c = pl02, pl15 = pl11, wep03/18 = wep02, wep20 = wep11, wep21/24 = wep09, wep25 = wep14,
  wep31/32 = wep10, wep46 = wep13; the module is configured once, under the first name), 11 (pl00, pl01,
  pl03..pl05, pl07..pl10, pl12, pl21) have none. Module ids are unique among the distinct RELs.
  Not on disc 1 although a `Bio4.<mod>.sym` exists: em06, em09, emmark, st0, st3_0..st3_3, pl03/10/12.
- Originals: `orig/G4BE08/files/{Rel,em}/<mod>.rel` and `orig/G4BE08/files/Bio4.<mod>.sym` are
  untracked. `python3 tools/extract_orig.py config/G4BE08/config.yml <disc>` writes them (plus
  `sys/main.dol`, `sys/main_split.dol`, `files/Bio4.sym`) from a disc image (.iso/.gcm, read directly)
  or from a directory made by `dtk disc extract`; `configure.py` runs it itself when a configured
  module's REL is missing and an image sits in `orig/G4BE08/`.
- DRS archives (`tools/drs.py list|rel|extract|pack|rebuild|roundtrip`, byte-identical round trip on
  all 128): a 0x20-byte free-text signature ("ハカセのアホーーーーーーー！！！" in Shift-JIS; 11 pl
  archives repeat the half-width "ﾊｶｾ " instead), then 32-byte records `{u32 type, size, 0, offset,
  p0, p1, 0, 0}` ending with type 0xFFFFFFFF, zero to 0x400. Record type 0 is the body at 0x400 (its
  size 0x20-rounded), type 4 the sound bank appended after it (or `{0xFFFFFFFE, 0, 0, file size}` when
  there is none). Body: `u32 count, rel_offset, 0, 0; u32 offsets[count]; char tags[count][4]`, padded
  to 0x20, then the entries (0x20-aligned, tags BIN/TPL/FCV/SEQ/EFF, zero tag = empty entry) — no sizes,
  an entry runs to the next offset and was padded with 0xCD (the packer's uninitialised buffer);
  `rel_offset` (0 = none) is the REL, its own size is the end of its last relocation list, padded with
  0xCD to 0x20. The sound bank is the same container again (records type 1 and 2, exact sizes, zero
  padding, p0/p1 bank parameters). `drs.py rebuild <orig.drs> build/G4BE08/<mod>/<mod>.rel <out.drs>`
  is the archive with a rebuilt REL (identical to the original for every module today).
- Module names are the disc file stems; a module's units are `<mod>/<file>.cpp` (source `src/<mod>/<file>.cpp`,
  or a shared source given in `config/G4BE08/modules.py`), objdiff calls the unit `<mod>/<mod>/<file>`,
  its target asm is `build/G4BE08/<mod>/asm/<mod>/<file>.s`, the split object `build/G4BE08/<mod>/obj/...`.
- Symbols: `config/G4BE08/modules/<mod>/{symbols.txt,splits.txt,sym_map.tsv,rel.json}`, generated by
  `python3 tools/gen_rel_config.py config/G4BE08/config.yml orig/G4BE08/files` (safe to re-run: names and
  scopes already synced from compiled units are kept). The debug `Bio4.<mod>.sym` only lists the `.text`
  functions of the module (offset, size, scope, demangled name; every function is attributed to
  "<mod>.preplf", so there is no per-object information) — data symbols are labels at every relocation
  target (`lbl_<mod>_<section>_<off>`), unnamed functions are `fn_<mod>_<off>`; em3e has no .sym at all
  (labels only). Addresses in the module files are section offsets.
- Units: a module is one unit `<mod>/<mod>.cpp` unless `UNITS` in `config/G4BE08/modules.py` names
  boundaries (`(unit, first function[, shared source[, {section: data start}]])`); the generator
  attributes .rodata/.data/.bss ranges to units by the relocations coming from each unit's code: a
  unit's data starts at its first reference above everything earlier units address (references
  below that must hit a global of an earlier unit), unreferenced data goes to the preceding unit
  unless the 4th element pins the start. The `__static_initialization_and_destruction_0`/`global
  constructors keyed to X` pairs and the `"D:/Bio4/Prog/<file>.cpp"` HALT strings are the best hints
  for the original file boundaries.
- Shared enemy library: the 16 Ganado modules em10..em17, em19..em1f, em20 (.text 0x4402C..0x44444)
  are `em10.cpp` (`"D:/Bio4/Prog/em10.cpp"`, cEm10 and the em10*/em1c*/plem10* helpers: .text
  0..0x43518 = 382 functions + the 0x3B8-byte template gap to 0x438D0, .rodata 0..0x1EC4, all 0x994
  bytes of .data, the 0x34-byte COMMON .bss — byte-identical in all 16, the split object
  `build/G4BE08/<mod>/obj/<mod>/em10.o` is the same in every module) followed by ONE per-enemy
  object, unit `<mod>/<mod>_set.cpp` (src/<mod>/<mod>_set.cpp, all 16 Matching): `_prolog` =
  `OSReport("em10 prolog Ok\n")` in every module + `EmInitFunc = EmXXInit; Em10SetFunc = EmXXSet;`
  (no ctor loop), `_epilog`/`_unresolved` empty, then EmXXInit (`new (em) cEm10()`), EmXXSet,
  EmXXWeaponSet (0x75C..0xB74 bytes). em1d/em1e/em1f/em20 include light.h (+ esp.h for the
  trailing `EspDataLoad((u32) ARC(0x278), 0xCD, 0)`): their `.rodata` is [light.h string][prolog
  string][five cManager<cLight> template strings] with the 0x3B8 cLight linkonce block after
  EmXXWeaponSet - only one TU lays it out that way (a separate entry object would put the template
  strings and the block before EmXXInit), which is why the former `<mod>_prolog.cpp` unit was
  merged. The real file name is not in the binary. The 28 other enemies (em18, em21..em3d,
  em3e; .text 0x10C8..0x17034) start with `_prolog`/`_epilog`/`_unresolved` (same code, 4 `_prolog`
  variants), then their own `"D:/Bio4/Prog/emXX.cpp"`, and share only cUnit's inline
  `beginEvent`/`endEvent`/`~cUnit`/`operator delete` (byte-identical, at the end).
- Compiler flags: `cflags_game` + `-G 0` (no small data in RELs: every DOL global goes through
  `lis/addi`), plus the module's `CFLAGS` entry of config/G4BE08/modules.py (Sscrn:
  `-fno-implement-inlines`, see the Sscrn subsection). Linkonce functions are placed at assembly time by
  `tools/ngccc.py place_linkonce_module` (the REL linkonce rule, Sscrn subsection); post-build
  `fold_linkonce.py --module <mod>` appends the vtables (see "Multi-object modules" below); no
  strip_unused (nothing is dead-stripped in a -r link).
- Toolchain: the original RELs came out of `ngcld -r` followed by SN's `snmakerel` (Nintendo's makerel
  port). We do the same: `tools/link_rel.py` links the units with `ngcld -r -T config/G4BE08/rel_ldscript.ld`
  (SN's preplf.ld layout: every section at 0, `_ctors`/`_dtors` labels and the `LONG(0)` terminators come
  from the script), `tools/make_rel.py` writes the REL from that ELF plus `rel.json` (module id, original
  ELF section count, string-table name offset/size, align/bss_align, REL section indices, imported
  modules). dtk's `rel make` was not usable: it keeps REL14 out of the table, takes REL section indices
  from our ELF and DOL section bytes from our main.elf, and knows nothing about the SN specifics below.
- What snmakerel did, reproduced in make_rel.py (verified byte-for-byte on all 110):
  - imports ordered other modules ascending, then self, then the DOL (fix_size = start of the self list);
    each list by section then offset; NOP entries for gaps > 0xFFFF.
  - REL24 to a same-section target is resolved and dropped; REL24 to another module/the DOL is patched
    to `bl _unresolved` and kept; REL14 (NgcAs emits one for every conditional branch, GAS does not)
    is resolved and kept.
  - ADDR32/ADDR16 fields hold S+A for local symbols and only A for globals, so symbol scopes matter:
    the generator derives them from the original fields; the few targets referenced both ways get
    `field_overrides` in rel.json. Our ngcld 3.9.3 -r also adds the displacement of the input section
    defining a *global* to the field (visible only from the second object on: em10's `_prolog` ->
    `Em10Init`), the original linker did not; make_rel writes A back into every global field. So a
    zero field in the original never tells whether the reference crossed an object boundary.
  - module-0 relocations carry the *original main.elf's* section index in the section byte (.text 2,
    .rodata 5, .data 6, .bss 7, .sdata 8, .sbss 9, .sdata2 10); make_rel resolves DOL names through
    `config/G4BE08/symbols.txt` (not main.elf, whose names follow the compiled DOL units).
  - undefined names resolve to the first definition in DOL, then linked modules by ascending id
    (t_camera's `__builtin_delete` is the DOL's although Tools/t_esp have copies).
  - ngcld 3.9.3 appends its BSS_TAG object: a pointer at the end of .data (`__sn__bss__tag__address__`,
    field 0) to the zero-size `__sn__bss__tag__` at the end of the objects' .bss; both are in every REL,
    so the split of the last unit skips that .data word.
  - g++ 2.95 emits uninitialised static data members as COMMON (template statics of cManager<T> etc.;
    the DOL's `IDSystem::m_scrn_mat` is one). `ngcld -r` leaves them unallocated and snmakerel appended
    them to .bss after the tag: 0x34 bytes in most modules with .bss (0x30 in em21/pl0f, none in the
    weapon/player modules). The skeleton carries them as one `.comm common_<mod>` (a `common` split,
    in the unit that addresses the block or has templates); make_rel allocates COMMON symbols after
    .bss in symbol-table order. Without a COMMON block the tag pointer targets the end of .bss, where
    the generator puts a local `__sn__bss__tag__` label for dtk.
  - dtk needs REL14 relocations against the section symbol (it emits them against the containing
    function, and ngcld's A-P patching then overflows on big modules): link_rel.py rewrites the split
    objects' REL14 entries into `objfix/` copies before linking.
- Proof unit: `src/st2/st2.cpp` (`"D:/Bio4/Prog/st2.cpp"`: `set`, `setTbl`, `_prolog`, `_epilog`,
  `_unresolved`), the same object at the end of every st2_* module; st1_*/st4_0 end with st1.cpp/st4.cpp.
  `_prolog` runs `_ctors`, calls `setTbl` (registers every stage room's Init/Main in the DOL's
  `St2_data_tbl`, cross-module references), `_unresolved` is `HALT()` at line 169.

### Workflow for one module unit (`st2_4/r22c`)

```sh
python3 tools/unit_info.py st2_4/r22c                 # functions, sizes, match % (module sym_map)
sed -n '/^\.fn NAME/,/^\.endfn/p' build/G4BE08/st2_4/asm/st2_4/r22c.s
# write src/st2_4/r22c.cpp, then:
ninja build/G4BE08/src/st2_4/r22c.o
python3 tools/sync_rel_symbols.py build/G4BE08/src/st2_4/r22c.o   # module symbols.txt (+DOL/imported modules for references)
python3 configure.py && ninja                          # must still print `111 files OK`
python3 tools/fdiff.py st2_4/r22c <mangled_symbol>
```

Then `MATCHING["st2_4/r22c.cpp"] = True` in `config/G4BE08/modules.py`, `python3 configure.py && ninja`.

Data of another (not yet compiled) unit of the same module has no name in the `.sym`, so a reference
like `Em10SetFunc` (em10.cpp's `.data+0`) or `_vt.5cEm10` cannot be matched by demangled name;
`sync_rel_symbols.py` resolves such undefined names through the relocations instead: the unit's split
object (`build/G4BE08/<mod>/obj/<unit>.o`) must reference a `lbl_`/`fn_` placeholder at the same
`.text` offsets with the same relocation types, and that placeholder is renamed. It reports the
names it could not resolve; a name whose relocations do not line up (function order differs from the
target) stays unresolved and `make_rel` then fails with "undefined symbol".

### Adding a module (every REL of disc 1 is configured; this is for another disc/build)

1. Append to `modules:` in `config/G4BE08/config.yml`: `object: files/<Rel|em>/<mod>.rel`, `name`,
   `splits`/`symbols` paths under `config/G4BE08/modules/<mod>/`. The object path says where the REL
   comes from: `files/Rel/` is a loose disc file, `files/em/<mod>.rel` is the REL inside
   `files/em/<mod>.drs`. Name a REL after the first archive that carries it (`tools/drs.py list`).
2. `python3 tools/extract_orig.py config/G4BE08/config.yml <image or extracted disc>` (also copies the
   `Bio4.<mod>.sym`; `configure.py` does this itself when an image is in `orig/G4BE08/`).
3. `python3 tools/gen_rel_config.py config/G4BE08/config.yml orig/G4BE08/files`; every module it
   imports must be configured too (the tool says which id is missing).
4. `sha1sum orig/G4BE08/files/<dir>/<mod>.rel` -> a `<hash>  build/G4BE08/<mod>/<mod>.rel` line in
   `config/G4BE08/build.sha1`; `python3 configure.py && ninja` must report every file OK. A mismatch
   is a new snmakerel/ngcld case for `tools/make_rel.py` (the `--verify` output names the region);
   never patch bytes by hand.

### Multi-object modules (t_emlist, st1_0, the tool modules)

- Unit boundaries from the `.sym`: the `"D:/Bio4/Prog/<file>.cpp"` HALT strings, function-name prefixes
  and the `.rodata` header-string groups (`light.h`/`atari.h`/... appear once per object that includes
  them, at parse time, i.e. before that object's function strings; template strings at the object's end).
  t_emlist = t_emlist.cpp (0x0..0x6388), t_prim.cpp (0x6388), t_util.cpp (0x6B2C), tools.cpp (0x7330);
  st1_0 = r100.cpp, r120.cpp (0x40B0), st1.cpp (0x5D28). Data the code never addresses (header strings,
  `.data` of a dead inline) is assigned with the 4th UNITS element `{".rodata": start, ".data": start}`.
- Shared sources: `src/tools/tools.cpp` (`_prolog` = ctors + `ToolsTask()`, the `DebugMenuSelected`
  switch of 23 Tool* entries, `_unresolved` HALT at line 142; the headers are included *after* the
  functions — its `.rodata` has the entry-point strings before atari.h/light.h/event.h/ctrl.h) is the
  last object of every tool module (byte-identical in t_emlist/t_camera/t_light/t_sce/t_event; in
  t_esp/Tools/t_id the linkonce orphan sections follow it, t_movie has more objects after it).
  `src/st1/st1.cpp` (29 rooms, HALT line 144) ends st1_0..st1_3. `src/tools/t_prim.cpp` / `t_util.cpp`
  are the full versions of the DOL's dead-stripped game/t_prim, game/t_util (different builds per tool
  module: t_light's t_util has only Init/QuitDefault, t_camera's t_prim only 5 functions).
- Linkonce in the module link (`fold_linkonce.py --module`): `ngcld -r` kept *every* object's
  `.gnu.linkonce.t.*` copies, appended to that object's .text in emission order, and the weak names
  resolve to the module's first copy — the `.sym` names only that copy, later copies are nameless
  `fn_<mod>_<off>` blocks (0x3B8 = log/countActiveWork/create(int)/create()/create(int,u32) of
  `cManager<cLight>` for every object that merely includes light.h). The fold keeps the copies the
  module sym_map names for the unit (demangled name + size) or, for a unit with none named, all of
  them as nameless code with weak-undefined symbols. The first copy's body set is *not* what our
  compiler emits for the same include set (t_emlist.cpp has countActiveWork + create(int) only, ours
  gives 5): the fold drops the rest, sizes decide.
- `sync_rel_symbols.py` resolves overloads by size: a mangled name already carried by a DOL or module
  sym_map row (`create__t8cManager1Z6cLighti` = 0x158) only claims a placeholder of that size, and an
  entry that already carries another mangled name is never renamed (message "left alone").
- Field rules seen with displaced sections: a *global* symbol's ADDR16/32 field is A (make_rel writes it
  back), a *local*'s is S+A — so scopes are visible: t_util's `globalCamera` is `static` (field 0x140),
  its five flag backups are globals (field 0), t_prim's Vrect/Orect/FlipMode/`bl` are statics.
- `GXWGFifo` (absolute, `include/gx.h`) is defined in `config/G4BE08/rel_ldscript.ld` too; ngcld -r keeps
  the relocations against it, `tools/link_rel.py` applies and drops them after the link (the RELs have
  the final `lis 0xCC01`/`-0x8000` words and no relocation).
- `.bss` of a compiled module unit is invisible to the split object compare (NOBITS); check sizes with
  readelf, and remember unreferenced `.bss` objects survive the -r link (t_prim's 0x20 behind
  ToolBuffer, dropped by the DOL link).
- t_emlist.cpp idioms (src/t_emlist/t_emlist.cpp, 36/53 functions): the work pointer is a struct member
  (`EmList.wk`, a one-member struct in .data right before the routine table): every store through it
  reloads the pointer, consecutive loads share it. `EmListCtrl* ctl = &EmList;` declared at the top of
  emlist_init puts the `lis EmList@ha` into the prologue (callee-saved) although the only store is
  behind seven calls. `y = 0x50 + i * 0x10;` as the *first* loop statement gives the giv init last in
  the preheader while `mr r4, y` survives in a branch where `i` is known (a plain `y += 0x10` counter
  inits first, an inline expression folds to `li 0xa0`). `int r = p->room; ((step + r) & 0xFF) |
  (r & 0xF00)` puts `step` first in the `add`; the member read directly puts the load first. Tool-style
  raw offsets: `(u32*) (no * 0x20 + (u32) pG + 0x501C)` + `BitOff(tbl[i >> 5], m)` = `slwi; add idx,pG;
  addi 0x501c; lwzx/stwx` with pG reloaded per iteration. `int step = wk->step; switch (step)` keeps the
  switch register and stores it as the constant in `case 1:` (`stw r10`). OPEN: emlist_file_menu_disp
  copies the `i - 11` giv into r8 once before the compare tree and shares one eprintf tail between the
  omake and stage cases; `switch (i - 11)` with `i - 11` in every call reduces the giv (`li r28, -0xb`)
  but cse folds the argument per case.

### Stage (room script) modules st1_0..st1_3, st2_0..st2_4, st4_0

- Layout of every stage REL (config/G4BE08/modules.py UNITS): `em_wrap.cpp` (src/st/em_wrap.cpp,
  include/em_wrap.h: cEmControl/cEmPatrol/cEmGuard + the cEmWrap enemy handle with ~50 members + free
  `setEm`/`SceCkFindPL`; no __FILE__ string, the real file name is unknown), then `cSceObj.cpp`
  (st2_0/st2_3/st4_0 only, src/st/cSceObj.cpp, "D:/Bio4/Prog/cSceObj.cpp"), then one object per room
  (`rNNN.cpp`, sources in src/st1/, src/st2/, src/st4/ — r102/r103/r108 are the same object in st1_1 and
  st1_3, so room sources are keyed by stage, not module), then st1.cpp/st2.cpp/st4.cpp (st4.cpp includes
  map_obj.h/light.h/widget.h/atari.h, hence header strings + a cManager<cLight> block after its code, and
  defines the global `st4_initAdaGame` that r405 calls). Room .text starts are the first function of the
  room (usually `RNNNInit`; r10a.cpp starts at EmSetNormal, r208.cpp at setResetNum (offset, the name is
  also in r222.cpp), r210.cpp at asl_wait, r40a.cpp at r40a_DuraluminCaseOpen, r405.cpp at snd_tbl_set —
  a `bl` to a function of the *previous* unit with an S+A field in the REL means the boundary is wrong;
  make_rel --verify shows it). Room .rodata starts are pinned at the room's first header string (each
  room's group is `[cFlag.set() string][atari.h/event.h/map_obj.h/light.h/widget.h...][HALT][flag_rsf.h]
  [rNNN.cpp]`, header strings are unreferenced); pointer tables referenced only from data need a pinned
  .data start too (st2_3 r21b.cpp: .data 0xA8).
- **The original REL link dead-stripped em_wrap.cpp and cSceObj.cpp at function level** (like the DOL's
  SDK objects: bodies gone, every string and constant pool kept, functions in declaration order): each
  module keeps exactly the members its rooms transitively call (st1_0: none, 0 bytes of .text; st2_4:
  six). `tools/strip_unused.py --gcc --module <mod> --unit <mod>/em_wrap.cpp` reproduces it from the
  module's sym_map.tsv (modules.py `STRIP_UNUSED` names the units; overloads are told apart by size once
  the row carries a mangled name, and anything still referenced from surviving code is kept — NgcAs
  writes intra-object `bl`s as `.text+off`). Room objects were not stripped (st4.cpp keeps the unused
  static helper... which turned out to be referenced from r405). em_wrap.cpp exists in two revisions:
  st2_4/st4_0 have no cEmControl/cEmPatrol/cEmGuard (no "cEmControl::SetPatrol" string, no 0.0/1000/100
  pool: src/st/em_wrap_v2.cpp = `#define EM_WRAP_NO_CONTROL` + include). Members that no module kept
  (isTrans, setMove, ..., get_l_pl) are written from their strings/pools only.
- em_wrap idioms: the getters are `if (isAlive() == 1) return pEm->x; err(...); return 0;` (fail block
  laid out first with `beq`); `checkStatus` is `return pEm->checkStatus(stat) != 0 ? 1 : 0;` inside the
  `if` (the ternary keeps the unmerged `li r3,0; b end`); `setPtr(cEm*, int)` stores `alive = a; no = -1;
  pEm = em; list = -1;` with `int a = (be_flag & 0x201) == 1` (one `li -1` for both narrow stores);
  `setPtr(s16, s8, int)` is `if (p != 0) return setPtr(p, errOn); return setEm(...)`; the patrol wrap is
  `if (next < 0) wrap = nPoint - 1; else wrap = (next > nPoint - 1) ? 0 : next; p->cur = wrap;` (fresh
  variable, one store); SceCkFindPL computes `cEm* p = pArray + size * i` *before* `cEmWrap em;` (the
  ctor call) and needs `Vec v` at frame offset 0 in cEmGuard::TaskMove for the recomputed `addi r4,r1,8`.
  OPEN: cEmWrap::setEm's `add r9, pG, idx` operand order / register choice (ours `add r9, idx, pG`; ~15
  forms of the EM_LIST access tried) — the only non-identical bytes in st1_1..st2_3/st4_0's em_wrap.
- SOLVED (r10d .rodata order): it was a unit-boundary error, not an include-order one. The generator
  gives unreferenced data to the preceding unit, so the `HALT %s(%d)` string that opens r10d.cpp's
  group (`[HALT][flag_rsf.h][r10d.cpp]`, like every room) had been attributed to r10c.cpp (r10c's own
  group ends with the cManager template strings) and r10e's HALT to r10d. Pins moved to 0xE40/0xE80
  in modules.py; both rooms are Matching. Rule: a room's `.rodata` pin is the *first* string of its
  `[cFlag.set()][atari.h ...][HALT][flag_rsf.h][rNNN.cpp]` group — a HALT string right after another
  room's template strings belongs to the next room. Check: dump each split object's `.rodata`
  strings (`readelf -x .rodata build/G4BE08/<mod>/obj/<mod>/rNNN.o`) and make sure no room group
  starts with `D:/Bio4/Prog/flag_rsf.h` or ends with a lone HALT after the cManager strings.
- Room-script idioms (src/st1/r10e, r11a, r109, r107, r102, r10a matched; r108 15 functions, 9 exact):
  - Every room includes `include/st_room.h` after main_mem.h: it defines the module's 0x34-byte COMMON
    placeholder (`asm(".comm common_<REL_MODULE>,52,4")`, REL_MODULE is a per-module `-D` configure.py
    adds to every module unit) so a module whose common-owning room is compiled still links (the split
    skeleton's `common_<mod>` merges with it), plus the scalar reference setters U8Set/U16Set/U32Set/
    IntSet/FAdd/FSub (FSet is global.h's).
  - `include/flag_rsf.h` is the original's shape (RsfSet/RsfClear/RsfCheck with `if (no > 0x1F) HALT`
    at lines 17/21/25; constant `no` folds the check away): `RsfCheck(G_ROOM_ID, 0)` is
    `lwz 4(r3); cmpwi 0; bge/blt` (sign test), higher bits `andis.`. `if (RsfCheck(..) == 0)` and
    `if (RsfCheck(..))` are the two branch polarities.
  - Room work: `static RNNNWork* rNNN_work; rNNN_work = (RNNNWork*) MEM_CALLOC(sizeof, 1, 0xd);` with
    `#line N "D:/Bio4/Prog/rNNN.cpp"` (N from the mem_calloc line argument). Stores into the work after
    a call reload the work pointer: write `rNNN_work->field = call(...)` directly (r109), and a store the
    original reloads *both* the work and the field after is a reference store (`PSet(rNNN_work->evd, ..)`,
    r102). Work pointers of `cSat*`/`cObj*` etc. get their own typed `PSet` per file.
  - `SceExec(0x12, (TaskFunc) fn, 0, 0, 2, 0)` / `SceAtDataSet_exec(no, 0x12, 0, (TaskFunc) fn, 0, 1|2)`
    are the task registrations; `EmSetFromList2(no, 1)`, `setEm(no, -1, 1, 1, 1)`, `SceCkFindPL(0)`,
    `SceCountEmAlive(lo, hi)` the enemy calls; BGM/stream tasks are `for (;;)` loops with an `on` flag
    whose `on = 1` is written *after* the SndRoom* call (the `li` then lands among the call's `li`s).
  - `Vec ang = {a, b, c}` locals with all-constant initialisers are `.rodata` templates copied with
    3 lwz/stw (r109, r102 `pos`); a Vec built from `stfs` of pool constants is memberwise stores, and
    one member stored through a `Vec* pa = &ang` pointer while the others are direct gives the
    `stfs f31, 4(r31)` / `stfs f0, 0x20(r1)` mix (r102 `ang`). A float kept in a callee-saved FPR across
    a call and stored after it is a local `f32 ry = K;` declared before the call. `cPlayer* pl = pPLS`
    (struct view of pPL) keeps the pPL load below the preceding Vec template stores.
  - Vec/aggregate locals are temp slots rounded to 16 bytes (a Vec takes 0x10 of frame); address-taken
    scalars (`cEm* torch; getRoomEtcTorch(0, &torch, 1)`) get their slot after every aggregate.
  - Angle constants: some are `deg * (PI / 180.0f)` folds (r102: -5.4/-3/-1.2 deg), others only
    reproduce as raw float literals (r109's Vecs) — use `numpy.float32(...)` shortest repr literals.
  - `const f32 step = K;` at the function top puts K first in the constant pool while the uses stay
    literal (r102 openCover, r108 openCover); `f32 w = 1000.0f;` (variable) before a float-heavy call
    makes it the first pool entry and the shared `fmr` source (r108 YarareInitCube).
  - Scroll objects: `SmdGetObjPtr(id)->be_flag |= 2` chains (r109 koya_init) are plain stores;
    `BitOn(obj->be_flag, 0x20)` (reference) is needed where the original reloads another global
    pointer after the store (r108 initPuzzle), and `static inline void setObj(cObj*& o, u32 id)
    { o = SmdGetObjPtr(id); BitOn(o->be_flag, 0x20); }` (reference parameters) puts every `lis` of the
    pointer globals into callee-saved registers before the first call.
  - Event flag words `pG->flags_174[i]` in the rooms are `*(u32*) (((no >> 5) << 2) + (u32) &pG->flags_174)`
    (cast-then-deref: the store forces a pG reload per loop iteration, r108).
  - `EmListData d; d.id = ..; d.type = ..; ... EmSetEvent(&d)` blocks: the type byte is stored before
    the halfword/word fields (own QI constant), `hp`, `x1A`, `xB` after `rot` (r10a).
  - `0x150 - cMes.getWork()->fontH - cMes.getWork()->lineSpace - 1` (sce_com's source) compiles to
    `0x150 - lineSpace - fontH` with our cc1plus; the rooms (r108 checkDoor/execPuzzle) need the
    operands written `lineSpace` first to get the target's `lbz 0x19; lbz 0x77` order — sce_com has the
    same mismatch in its own object.
  - Room-local static tables in `.data` (r108 `R108Symbol r108_symbol[8]`, r109's Vec positions) are
    non-const initialised statics; a table only read is `static const` and lands after the template
    strings at the end of `.rodata` (r11a/r107/r10a `AtEffInfo`).
  - cEm27::setWaterHeight (em27 module) is declared in include/em27.h; getRoomEtc*() in etc_model.h;
    EventMgr::NameChange/SetEvt(void*, u32*) in event.h (the second SetEvt overload at 0x8013991C
    takes a name; DOL symbols renamed by hand); EspDataLoad/EspGetEfmTplAddr in esp.h; EmSetEvent in
    em_set.h; EmReadSearch in read.h.
  - OPEN (r108): execShowView issues `lfs f1, 0.0` last (ours second) around the RsfSet store;
    checkEmReset's `int list[11]` end pointer is `addi r29, r31, 0x28` from the array pseudo (ours folds to
    `r1+0x30`; `int* p = list` forms tried); openCover's tail after the `do {} while (1)` loop re-materialises
    the `lis` of both cover globals and the 220.0 constant instead of reusing the loop-hoisted registers;
    switchSymbol / str_check / initChurchBell differ only in callee-saved register choice or one
    load order.

### Ganado per-enemy objects (em10..em20 `<mod>/<mod>_set.cpp`, all 16 Matching, 2026-09)

- The 16 sources were generated from the target asm (the tables differ, the code does not); idioms:
  `Em10Work* w = EM10_WK(em); switch (em->type) {...}` with one arm per model type, each arm a run
  of `w->mot[i] = PL_ARC_PTR(em->subArc, N)` (`lwz subArc; lwz ofs; add; stw`, the subArc reload
  after every store is the natural aliasing of a store through `w` against a load through `em`),
  ending in `Em10SetSeTbl(em, K)`; then `w->x6C5 = K` (or `if (em->type == 6) w->x6C5 = 0; else
  w->x6C5 = 1;` for the `li 1; bne; li 0; stb` shape) and `EmXXWeaponSet(em)`.
- Archive stores are chained (each reload depends on the previous store), so their issue order IS
  the source order (types 6 fill `mot[16]`/`mot[17]` between `mot[4]` and `mot[5]`). Zero stores are
  free: write them after the archive store of the *segment* they are issued in (the run between two
  `lwz subArc`), ascending by index; the dying zero store (`stw r11, 0xd0` = mot[40]) then comes
  out first or mid-run exactly like the target.
- Compare tree: the default arm's `em->type = K` store means `case K:` is a label of the default arm
  (the node is real, its `beq default` is jump-threaded into `ble/blt default`: em11 `case 7:
  default:` gives `cmpwi 8; beq; ble def; cmpwi 9; beq`, em12/em17/em13 `case 0: default:`, em1c
  `case 7:`, em10 `case 0:`). em14 has `case 7:` as the tree root (`cmpwi 7; beq default`).
- Variant types (em1d/em1e/em1f/em20): `case 15: case 19:` arms start with a reloaded `if (em->type
  == 15) {mot[0], mot[1] = A} else {= B}` and share the rest; the default arm `case 14: case 18:
  default: if (em->type == 18) {A} else {B; em->type = 14;}`. em20's `case 18` is a label INSIDE the
  then-arm (`if (em->type == 18) { case 18: A } else {..}`: the switch jumps past the compare).
  em1d's types 17/21 have no compare and share everything from `mot[2]`: `case 17: A; goto common17;
  case 21: B; common17: ...` - two full copies compile to a different zero-store schedule in the
  case-21 block (it would start at `mot[0]`; the target's starts at `mot[2]`), and the cross-jump
  runs after sched1.
- WeaponSet with `lbz type; cmpwi 4; bne` = `if (em->type == 4) { mot[67] = ..; [mot[68] = ..] }
  else {..}` with the identical `add; stw` tail cross-jumped (em10/em15/em16; em16 differs in two
  slots). em20's case 2 also does `em->flags_3C8 |= 0x10000000;` before its `Em10SetSeTbl`.
- `Em10SetFunc` is em10.cpp's `.data+0` (declared in em10.h); `EmInitFunc` is game/em.cpp's
  (`extern void (*EmInitFunc)(cEm*)`); `Em10SetSeTbl` is `extern "C"`.

### Sscrn (sub screen DLL, src/Sscrn/ss_*.cpp, include/ss_main.h)

- The screens are `Widget<SUB_SCREEN>` state-machine nodes (include/widget.h: `num`, `link[]`, `cur`, vptr
  at 0xC; virtuals dtor/init/quit/move; `connect(no, w)`, `transit(no, wk)` with the two `pLog->err`
  strings). `SUB_SCREEN` is the real tag of sscrn.h's work (`SubScreenWork` is a typedef): the module's
  mangled names carry it. The link table is `mem_alloc(4 * num, "widget.h", 89, 1, 13)`; derived widgets
  have NO user constructor (`Widget(int n = 1)` + implicit ctors: an in-class `SsX(int n) : Widget(n) {}`
  is emitted out of line, 0xA4 per class) and are declared in the order their vtables appear reversed
  (`_vt.9CapSelect` at the lowest .rodata address = declared last).
- Each unit's linkonce block is `[cManager<cLight> copies] ~Widget<SUB_SCREEN> [own synthesized dtors +
  in-class inlines in declaration order] Widget::quit, init, move`: the destructor is instantiated by a
  `static inline` `delete w` helper in ss_main.h before any derived class is declared, quit/init/move
  at their first use (transit uses quit then init). Units with named linkonce copies next to nameless
  duplicate blocks (ss_debug's dispWorkNum after the 0x3B8 cManager<cLight> block, ss_file's 0x408 +
  0xC around its dtors) were handled by fold_linkonce's `keep_unnamed` size sum; the module rule is
  now implemented in `tools/ngccc.py place_linkonce_module` (see next item), fold's module heuristics
  only remain for the legacy `--prodg-driver ngccc` path.
- **REL linkonce rule (SOLVED, tools/ngccc.py `place_linkonce_module`, applies to every module unit):**
  the original `ngcld -r` link kept the FIRST object's copy of a linkonce function only if some
  relocation in the module references its symbol, and every LATER object's copies whole as nameless
  code. Evidence in every module: the first unit including light.h names `cManager<cLight>::
  countActiveWork` + `create(int)` only (`bl`'d from the later blocks' `create()`/`create(int,u32)`)
  while `log`, `create()`, `create(int,u32)` (called only through DOL vtables) vanish, and every later
  unit carries the full 0x3B8 block (Sscrn ss_cap vs ss_main.., st1_1 r101 vs r102.., t_emlist vs
  t_util/tools, em10's own partial link: named pair + 0x3B8 block in one object). ss_main's
  `cManager<cMap>::log` is exactly such an unreferenced first copy (dropped), its cLight/Widget copies
  later duplicates (kept). ngccc.py assembles once for the sizes, then rewrites the asm: named copy
  -> `.text` in place; unnamed with an earlier unit naming the class instance -> `.text` in place
  with a local label (nameless duplicate, references resolve to the first copy); unnamed with this
  unit being the class's first instantiator -> deleted (its `.rodata` strings/pool stay, like the
  original); class instance named nowhere -> kept nameless (override: modules.py `LINKONCE_DROP =
  {unit: [mangled names]}`). `.text` in place matters because the original interleaves the template
  bodies with `__static_initialization_and_destruction_0` and the deferred inlines (ss_main: cLight
  block, cModelInfo/cParts/cMap templates, static init, LightSetModel2, ~Widget, dtors, quit/init/move,
  log/~cManager/destroy<cParts,cModelInfo>, keyed ctor/dtor).
- End-of-file output order (ss_main): a global function output AFTER the static-init function
  (LightSetModel2 at 0xD5B4) is a deferred `inline` whose address the code takes; it is output by
  `wrapup_global_declarations` in `saved_inlines` order. COMPILER-DIFF candidate #8: the original
  queues synthesized destructors when they are synthesized (end of file), our cc1plus queues them at
  the class definition (`cons_up_default_function` -> `mark_inline_for_output`), so to come out before
  the widget destructors the inline must be DEFINED before the widget classes are declared:
  ss_main.cpp defines `extern "C" inline void LightSetModel2()` between `#include "sscrn.h"` and
  `#include "ss_main.h"`.
- A group of functions that follows a unit's end-of-file blocks (synthesized dtors, Widget
  quit/init/move) is a separate object: the ss_Draw_tpl/line3d/tile3d helpers after ss_item are
  `Sscrn/ss_item_draw.cpp` (real name unknown; .rodata = three 0.0f pools + 4 pad, no header strings,
  so it includes only gx/tpl/trans/camera headers; `static` `_trans` callbacks inside `extern "C"`;
  `u32 blend` parameters give the `cmpwi 1/beq; cmplwi 1/blt; cmpwi 2; cmpwi 3` tree; the tpl range
  test is `(u32) tpl - 0x80000000 > 0x02FFFFFF`, the later pointer checks two separate `if`s).
- `.rodata` alignment: a unit with a vtable has an 8-aligned `.rodata` (`.align 3` of the vtable
  sections), one without (ss_debug) 4; the split objects are all `align:4`, so a compiled ss_debug
  loses the 4-byte pad before ss_file's `.rodata` until ss_file is compiled too — flip both together.
- Two identical strings are not merged when one comes from a template instantiation and the other from
  a parse-time inline (`"D:/Bio4/Prog/widget.h"` twice): route both through one `static inline const
  char* widgetFileName()` so the inlined copies share the SYMBOL_REF.
- `if (c) return 0; body; return 1;` puts `li r3,0` out of line at the end (jump1 turns the
  `set r3; use r3; jump ret` block into a return sequence); the target's `li r3,0` before the `bne`
  that jumps to the epilogue is `if (c) ret = 0; else { body; ret = 1; } return ret;` (or `goto`).
- `u64 Key.trg & bit` tests: bit 31 of the low word is `0x80000000` (`clrrwi 31`), not 1.
- A `switch` whose two arms come out as `cmpwi 1; beq L1; cmpwi 2; bne END; [case 2]; b END; L1: [case 1]`
  has `case 2:` written before `case 1:` (ss_debug bullet, ss_file SsFileMain::move).
- A shared `state++` tail entered from a `break`-ing case and from a falling case is a `goto NEXT` label
  (`case 0: ...; goto NEXT; case 1: if (..) break; NEXT: state++;`), otherwise cse folds the second
  copy to `li r0, 2` (SsFileInit::move).
- Debug menus: column positions are `int cx = 10;` assigned *after* the header `eprintf` (so the first
  call uses the literal 0x50 while the loop keeps `slwi r3, r23, 3`), the value column is `(cx + 13) * 8`
  (hoisted `addi`, folded to `li 0x17` by cse2), row y is the giv `0x9A + i * 0xE`; the cursor colour
  `int col = i == cursor ? 4 : 0;` is one loop-body local reused by a later case (ssDbgPzzl::move).
  A clamp on a global (`pG->x4F98`) that keeps the pG register across the diamond is
  `GlobalWork* g = pG; int p = g->x; if (p >= 0) { if (p > M) p = M; } else p = 0; g->x = p;`.
- Font sizes are `s16 x[2] = {0, 17}` / `s8 x[4]` statics passed as `x[1]`/`x[3]` to
  `setFontSize(int, s8, s8)` without truncation (COMPILER-DIFF 4: `MessageControlS::setFontSizeS`
  alias in ss_file.cpp); `IdNum.killI(0xFF, 0x40 + i)` likewise (id_sys.h).
- The DLL's model managers are `cSsPartsMgr`/`cSsModInfoMgr` (ss_main.h): constructed by the DOL's
  `__9cPartsMgr`/`__12cModInfoMgr` (asm-labelled ctors) but without a virtual destructor, so the static
  destructor inlines `cManager<T>::~cManager` (stores the cManager vtable) as the target does.
- Status: ss_cap, ss_debug, ss_file, ss_item_draw Matching (the REL is byte-identical with the four
  compiled); ss_main has 57/58 functions byte-identical (open: SubScreenTask register allocation /
  `lis pG@ha` hoisting / `cur->init(wk)` tail merging); ss_item is written (34 functions incl. dtors,
  25 byte-identical, .rodata/.data/.bss identical), open items below; ss_term (29/29 named
  functions, eof block open) and ss_model (40/47) are written, see their items; ss_map (src/Sscrn/
  ss_map.cpp, 88/105 named functions byte-identical, .rodata/.data/.bss identical since 2026-09:
  the former 8-byte gap was doorModelInit's missing 2^52 pool entry (`(f32) (int) e->ang` of the u8
  angle, the classic double trick, not a fast-cast psq_l) plus the two file-scope `static const`
  tables in the wrong order (map_cam_entire is defined before mark_model_tbl); see its item) and ss_pzzl (src/Sscrn/ss_pzzl.cpp, 45/64, .rodata/.data/.bss identical, second pass) are
  written; ss_shop (src/Sscrn/ss_shop.cpp, the merchant screen: 60/74 functions byte-identical incl.
  the 0x980 eof block, .rodata/.data/.bss identical, .text 8 bytes short) is written, see its item.
- ss_shop idioms (2026-09): include order light.h, map_obj.h, widget.h (the three header strings), then
  "ss_shop.dat" (SsShopInit::move) and the HALT string (mem_alloc lines 0x1BA/0x242). The 13 widgets are
  declared in the order SsShopInit, SsShopMain (ss_main.h), ShopTopMenu(3 links, ctor sets cursor = 1),
  SellMenuSelect(2), SellItemNum(2), SellConfirm(1), BuyMenuSelect(2), BuyItemNum(3, 0x20 bytes),
  BuyConfirm(3), BuyPuzzleEnd(2), LvUpMenuSelect(2), LvUpItemSelect(2), LvUpConfirm(1, 0x20); the link
  count / size of each `new` in SsShopMain::init belongs to the vtable stored AFTER it (the vptr store
  follows the `stw` of the member). SsShopMain::init also creates ss_pzzl's PzzlThinking / PieceSelect /
  CaseChange: their classes moved to include/ss_pzzl.h in the target's declaration order (PiecePopUp,
  PiecePopDown, PzzlThinking, PieceSelect, PieceCommand, PieceCombine, CaseChange = reverse of the
  .rodata vtable order 29E0..2B00; the old ss_pzzl.cpp order was wrong) and the nine pzzl / thirteen
  shop vtable labels were renamed `_vt.<Class>` by hand (the sync tool cannot resolve them).
  SUB_SCREEN gained pShop (0x204), x2B4, pShopWk (0x314, `ShopWork` 0x48) and pMerchant (0x318).
  Idioms: `tbl[(*num)++] = X` (getGreetMsg) keeps the `*num` value in a register across the `tbl[]`
  store (a re-read after the store reloads it); `if (a || b) tbl[(*num)++] = 1; else tbl[(*num)++] = 0`
  gives the `stwx r3` (the zero is levelNew's result) + shared `addi/stw` tail; two early `return 0`
  paths share one `li r3,0` only through `goto NG` to a `NG: return 0` at the end. `cMes.mes[result].flags2`
  (index = the member just zeroed) is what gives `lwzx r0, r9, rZERO` with the zero pseudo; `getMes(0)`
  folds. A function-level `IdUnit* u` reassigned by every `unitPtr()` call gives the `mr r9,r3` copies
  (closeCoat, SsShopMain::init); direct `IdSub.unitPtr(..)->flags` expressions use r3. Separate
  block-local loop counters per loop (SsShopMain::init: r29/r30/r27) keep `this` in r31. `state =
  greetIdx = greetStep = 0` (chain) reproduces `stb 0x10; stw 0x28; stw 0x24` after a call without
  reusing the pre-call QI zero. `if (x) transit(..); else ok = 0;` (then-arm = the call) lets jump2
  merge both `ok = 0; b join` copies out of line; `case 1: default:` shares the default body in the
  `str` switch. dispSellItemList / dispBuyItemList are hand-rolled goto loops (`i = top; goto TEST;
  BODY: ...; i++; TEST: if (i < end) { item = ..; pe = ..; row = i - top; if (pe) goto BODY; }`): no
  loop notes, so the 320/0.8/240 pool constants stay inside the loop and `row + 0x40` etc. are
  recomputed, while dispLvUpItemList is a real `for` (constants hoisted to f29-f31, four givs). In all
  three, the first `for (k = 0; k < 5; k++)` uses its own variable, `i = top` is assigned before the
  dispScrollBar call and `end = top + n` after it, the cursor mark is one `mark = unitPtr(0x3F)` with
  `if (cursor) |= 8 else &= ~8`, the `ItemInfo info` / `Vec pos` temporaries are block-scoped (the
  Sell frame: info 0x8, pos 0x10, second info 0x8, dispPrice's pos 0x8 via combine_temp_slots), the
  message slot is `u8 slot = row + 8`, and `ot`/`otNo` are stored through `U16Set` (ot first).
  The digit displays (dispPrice, stockNumDisp, weaponLevelDisp, levelItemDisp, SellItemNum::move)
  copy the number into a fresh `int n` AFTER the preceding `unitPtr()->flags |= 8` statement (the copy
  lands in r4 after the call: `mr r4, rNUM`); the leading-zero loop is `on = 0; for (i = N-1; i >= 0;
  i--) { if (on == 0) { if (digit[i] == 0 && i != 0) continue; on = 1; } u = unitPtr(base + i); u->flags
  |= 8; u->flags_7F |= 2; u->no = digit[i]; }`. weaponLevelDisp/levelItemDisp digit loop: `if (type ==
  3) { leading-zero skip } else if (type == 0 && i == 2 && digit[2] == 0) hide; else show;` (the
  type == 0 test survives because it is in the other arm); the bar colour is `src = unitPtr(3); if (i <
  lv) { int max = WeaponId2MaxLevel(..); src = colOn; if (lv > max) src = colOff; }` (max in a local
  before the assignments keeps src out of the call's live range, so it stays in r3); `switch (id) {
  case 0x40: .. case 0x34: ..}` (0x40 first) with `case 1:`/`case 2:`/`case 3:` written as separate
  `lv = 1` bodies. The ratio getters take the int level through `getPowerRatioI`-style asm aliases
  (COMPILER-DIFF 4, no `extsb`); `WeaponId2ChargeNumI` keeps the `& 0x1FFF` mask. LvUpConfirm::move
  writes the ItemWork::x6 nibbles through a `TuneLevel` bitfield view (`u16 fire:4, mag:4, speed:4,
  ex:4`) with `(u8) ((s8) sw->lv[0] - 1)` for the top nibble (`lbz +3; extsb; subi; clrlwi 24; slwi 12`)
  and `(s8) sw->lv[i] - 1` for the others; its message branch is `if (msg == 0x17 && (result =
  cMes.getMes(1)->result) != 0) {..} else if (msg == 0x18)` (the result == 0 path joins the second test,
  which therefore reloads msg and Key.trg). Message positions: `int x = (int)(..) + ofs_x;` before the
  MesSet with the y expression inline (loads ofs_x first; both inline loads ofs_y first and cost a
  callee-saved register); `shop_msg[msg]` read twice through the member (not a local copy). Clamps
  against 1 keep `cmpwi 1; blt` only through a variable (`int min = 1`), a literal folds to `<= 0`;
  the 0x54..0x55 test in the shop's itemTexNo is `id <= hi && id >= lo` with `int hi/lo` locals (a
  literal range folds to `subi/cmplwi`). `if (p->num >= left) { for (i = 0; i < left; i++) dump(p);
  break; } left -= p->num; dumpAll(p);` gives the reversed count-down dump loop with the subtract block
  out of line (SellConfirm). setOrientation: `m->rot.x = m->rot.y = m->rot.z = 0.0f; m->scale.x = .. =
  1.0f;` chains give the a0/ac/a8/a4/b4/b0 store order; the place table is indexed (`tbl[i].rot`) for
  the two givs. dispItem clears `be_flag & ~2` (rlwinm 0,31,29). screenPos2worldPos loads `pos.z` into
  a local before the `tan` call (f31). `sw->pShop = (SsArc*) (wk->aramSize + (u32) wk->pBuf)` (offset
  first). `MesData.setPtr(0, ..)` / `setPtr(2, ..)` give the `stwx r0, r9, rZERO` / `stw 8(r9)` pair.
  Open (register allocation / scheduling only unless noted): dispSellItemList & dispBuyItemList
  (`add end` scheduled after the dispScrollBar call and the call passing `top` not `i`; the
  frame/text `unitPtr(row+0x40)`/`unitPtr(row)` calls use a fresh `lis IdSub@ha` (Sell: one
  callee-saved r30 for both; LvUp: rematerialised `lis r9` per call) that gcse never unifies with the
  hoisted copy - a second `extern IDSystem IdSub2 asm("IdSub")` decl splits the expression but ours
  then hoists it), dispLvUpItemList (same `lis` + register names), levelItemDisp (-0x18: the same
  `lis`, `val[cur]` index form, digit/pos slot sharing), weaponLevelDisp (+4: `digit[2]` read via the
  array pseudo `lwz 8(rBASE)` where ours folds to `16(r1)`, register names), SellItemNum::move
  (`this` r24 vs val r25 swap; the fpmem address pseudos are callee-saved r29/r30 in the target),
  BuyItemNum::move (+8: the `SndCall(0,5)` tail of the cancel branch is cross-jumped into case 1's
  `li r8,0; bl` in the target), BuyConfirm::move (+4), LvUpItemSelect::move (item r28/r30),
  LvUpConfirm::move (-4: one `extsb` ours folds away), stockNumDisp / dispPrice (loop counter vs
  digit pointer registers r30/r31 swapped), screenPos2worldPos (x/z store order, scr/out registers).
- ss_map idioms (2026-09): include order light.h, map_obj.h, widget.h, atari.h (the cSat/Widget/
  cUnit vtables come out in that reverse order after the widget vtables). The unit defines its own
  `extern "C" inline LightSetModel2` before ss_main.h (the module's second copy, nameless 0x2C at
  the eof before ~cSat / ~Widget). Uninitialised statics are declared BEFORE `#include "ss_main.h"`:
  its externs of ssPlModel/ssWepModel (defined here) would otherwise put them first in .bss
  (first-declaration order). Small local arrays of <= 8 bytes (`int x[2]`, `int x[1]`, `u8 x[4]`)
  have an integer mode, get a pseudo at declaration and only receive a frame slot when their
  address is first taken (put_var_into_stack): their slots follow all BLKmode locals, in
  address-taking order (markGoalPosition: the `int[2]` table lands behind the 3/4/6-element ones
  although declared between them; the `int[1]` singletons follow in switch-case order), while
  their .rodata templates keep declaration order. `static const Vec` locals of a `static inline`
  helper defined between two callers are the single .rodata copy at the helper's position
  (mapModelLight before mapModelInit, after the mapColor tables); file-scope `static const`
  tables (mark_model_tbl, map_cam_entire) come out at the eof after the vtables. getAreaNo is a
  `switch ((u32) room)` with `case a ... b:` ranges (`cmplwi` trees), `case 0: default:` at the
  top and no trailing return (the surviving `return 0` copy is stage 3's inner default). `(x &
  (1 << n))` folds to `sraw/andi.`; `u32 bit = 1 << no;` keeps `slw/and.` (mapModeCheck). The
  8-float `f32 tbl[8]` .data statics are debug camera presets of which only [0] is read.
  Open: mapPositionCheck (-0x1C: the two cSat locals' ctor stores / debug-draw block layout),
  mapColor (+0x14 frame spills), doorModelInit (-0x30), mapModelInit/markGoalPosition/
  markMerchantPosition/markTreasureExist/markCoinDisp/markTreasureDisp (register allocation of
  the table copies), zoomMove (target has 8 more bytes of frame + one more callee-saved reg),
  mapChangeViewport (`map_vp_init = 1` store slot), MapModeSelect::init/move (timer store order,
  `lwz pG` placement), SsMapInit/SsMapMain::move (+8/+4).
- ss_pzzl notes (first pass, 2026-09): pzlBoard+0xC is a Mtx (puzzle.h `mat`, the board -> world
  matrix caseModelMove sets); the grid cell size is a static member of a local class
  (`pzlGrid::size`, the first word of the module's COMMON block, set to 100.0 by caseModelMove);
  the line width statics are TWO `u16` scalars per line kind (`x_ot = 0xF; x_prio = 0`, each
  loaded with its own `lis`; a `u16 x[2]` pair shares one base register); `.data` A44 is the
  message-open flag, AD0 a 20-byte unreferenced table behind an `int = 0`; the second
  `setCommandId` of the module is `static` here.
- ss_pzzl second pass (2026-09, 45/64 byte-identical, .rodata/.data/.bss identical): the Mtx copies
  are the word-copy loop, written as `MTX_COPY` = `MtxPtr d_ = dst; int i_ = 2; MtxPtr s_ = src;
  do { dp_ = *d_; sp_ = *s_; for (j_..4) *dp_++ = *sp_++; d_++; s_++; } while (i_--);` (the
  `li rX,2` of the counter is issued between the `d_` and `s_` inits in every copy of the unit, which
  the motion.cpp `while (i_--)`/`i_ = 3` form gets wrong for a `src` that needs an `addi`).
  caseModelMove: `const f32 size = 100.0f;` at the top (its pool entry precedes the 2^52 double;
  the store is `FSet(pzlGrid::size, size)` so the following `wk->x2B0->caseBoard` load stays below
  it), the `(f32) b->w` conversion is the double trick of an `int w = b->w;` local, two blocks share
  the frame (`{ Mtx tmp; Vec p; Vec ax, ay, az; }` then `{ Vec scr2; Mtx mat; Vec t; Vec p; Mtx
  mat2; }`: the freed block-1 slots are reused best-fit, the 0x10 remainder at 0xA8 stays a hole and
  mat2 gets a fresh slot), the board matrix is rebuilt column by column from three Vec axes
  (`ax.x = m->mat[0][0]; ax.y = m->mat[1][0]; ...; tmp[0][0] = ax.x; ...`) after a copy loop of
  m->mat, and only `m->mat[1][1]` is read through a `f32* col = &m->mat[0][1]` pointer (`lfs
  0x10(r6)` with the `addi r6, r28, 0x10` hoisted before the loop). Residual: one more callee-saved
  register (sw/u/parts allocation). Other idioms: the `y + 1`/`x - 1` neighbours passed to
  `getPiece`/`cellState`/`cell` are `(s8)` casts (`extsb` before the call, as puzzle.cpp does);
  `g = pzlGrid::size` is read after the copy loop and `item = 0` after the early return
  (drawCursor); `a.x = 0; a.y = 0; a.z = 0` in x, y, z order comes out x, z, y (drawGridLine),
  whose colour/ot/prio statics are read into `u32 col; u16 ot, prio;` locals before each loop pair
  (callee-saved, no reload after the calls) and whose board is read as `wk->x2B0->caseBoard->h`
  twice (a `pzlBoard* bd` local reassigned for the space board is a global-alloc pseudo in r3);
  cmpVer walks a `Vec* p = c->v; ... p++` pointer; screenPos2puzzlePos reads `pos.z` (not .y)
  into a local before `tan`; puzzlePos2screenPos computes `ang = fovy * 0.5f * 0.017453292f`
  into a local BEFORE `az = fabsf(out->z)` (the volatile asm is a barrier: constants expanded
  after it stay after it); the shared light set is one `static inline pzzlModelLight(m)` (single
  `static const Vec` pair between pieceModelDisp and pieceModelInit); pieceFrameDisp's
  `VECNormalize:[%s/%d]` uses `__FILE__` under `#line 1158 "D:/Bio4/Prog/ss_pzzl.cpp"` (one FILE
  string shared with SsPzzlInit::move's DVD_READ_N); pzzlEquipDisp has two unused 0x20-byte
  locals (`u8 x[0x20]` after `scr` and after `pos`: frame 0xB0, pos at 0x58, info at 0x88),
  `col = colorRRGGBBAA(..)` in a local before `pieceFrameDisp(arm->model, col, 3)` (a nested call
  argument makes ours precompute `arm->model` into a callee-saved register) and two separate
  `arm = pl->piecePtr(..)` calls in the if/else (cross-jumped); drawCursorInit is `const f32 big =
  1000.0f` (its `lis` kept in r28 across the calls) with the corners written v[0].x, v[0].y, ..
  v[3].y (dying-first puts v[3].y / v[2].x first); `if (c) ret = 0; else { body; ret = 1; }`
  (back2PieceSelect), `case 1:` before `case 0:` (tempSpaceDisp), `if (n != 0) { .. ret = 1/0 }
  else ret = 1` (isTerminable: the `n == 0` arm last); sscrn_pzzl_in/out_init use one reassigned
  `IdUnit* u`; itemCommandType's case 6 keeps an `int id = item->id` with `switch ((u16) id)` and
  calls `itemCombineCheckI` (item.h int view, COMPILER-DIFF 4) on both paths (an asm-labelled
  alias never cross-jumps with the plain declaration: the symbol_ref string differs). numDisp calls
  use the `numDispI` int view (no `clrlwi` of `0x40 + i`). Open: PieceCommand::move (-0x98: the
  nested `command_id` switch tables), PieceCombine::move (-0x10), PieceSelect/PzzlThinking::move,
  SsPzzlMain::init/move/quit, SsPzzlInit::move, pieceTblInit (the `piece_info` id switch tree),
  pieceModelOrientation, pieceFrameDisp, getPieceVertex, pieceModelDisp (`&scr` PRE'd, COMPILER-
  DIFF 3), pieceModelInit, pzzlCursorDisp (+4: `lwz r0,0x2b0; mr r3,r0` where the target loads
  `this` into r3 after the `mr r30,r3` col copy), setCommandId, itemCommandType (`lhz r0; mr r3,r0;
  clrlwi` of the raw u16 pass).
- **The module was compiled with `-fno-implement-inlines`** (config/G4BE08/modules.py `CFLAGS`,
  wired through configure.py's `REL_CFLAGS`): SubScreenTask creates every screen's Init/Main widget
  with per-class link counts (`SsFileMain` 5, `SsItemMain`/`SsPzzlMain` 6, `SsMapMain` 5,
  `SsCapMain` 2, `SsExitMain` 0), which needs in-class constructors
  `SsFileMain() : Widget<SUB_SCREEN>(5) {}`, yet no unit of the module has a constructor body (the
  .sym lists none, the nameless blocks are exactly the cManager<cLight> / ~Widget / quit-init-move
  copies). With the default flags g++ 2.95 emits every in-class inline member of a vtable-owning class
  out of line (`__10SsFileMain`, 0xA4); cp/decl2.c `import_export_decl` makes a non-virtual inline
  member external when `!flag_implement_inlines`, so the flag removes exactly those bodies while the
  vtables, synthesized destructors and virtual inlines stay (verified: the three matched units are
  byte-identical with and without the flag). All widget classes are now declared in ss_main.h
  (SubScreenTask needs their sizes and vtables); each unit's own classes keep their relative order
  (vtables are emitted in reverse declaration order per unit: ss_main's are SsExitInit, SsExitMain,
  SsItemExamine — SsItemExamine last).
- `static int file_wait[1]` (one-element array): the in-struct store `file_wait[0] = 0` keeps the
  following `state++` load below it and stops cse from folding case 1's `state++` to `li r0,2`; the
  case-0 block then has two pseudos and the `lis file_wait@ha` gets r11 instead of r9 (SsFileInit::move).
- A loop variable shared by two identical loops (`int i, n` at function scope for both language
  branches) gives both loops the `mr r10,r9; mr r11,r10` shape; per-branch locals lose them in the
  second loop (getTplName).
- Message slot address as one expression on a pointer variable, not the `getMes` inline
  (`SS_MES(pm, no)` = `(no) * sizeof(Message) + (u32) (pm) + sizeof(u32)`, `MessageControl* pm = &cMes`
  block-local): a reference argument built from it (`U16Set(SS_MES(pm, slot)->charSpace, v)`) is
  computed in place into the parameter register (three sets of one `reg/v`), which makes cse lose the
  `slot * 0xEC` product; the next `SS_MES(pm, slot)->lineH = zero` re-multiplies and gcse PRE turns it
  into the `mulli; mr r9,r11; add r11,r11,r3; add r9,r9,r3` pair (dispFileList, mes.cpp setLayout).
  The plain store with a `u16 zero = 0` local keeps `addi r9,r9,4; sth 0x76(r9)` unfolded; the
  reference store folds to `sth 0x7c(r11)`. The zero local declared next to the stores has lifetime
  >= 2 so loop.c hoists it (`li r19,0` in the preheader, threshold 71 x savings x lifetime >= 140
  insns); declared at the body top it is hoisted too but its `li` lands before the `lis cMes@ha`.
- Loop-body `int x, y` (block-local) give the loop its own pseudos: the title's x/y stay r30/r29 and
  the loop's get r28/r29 (dispFileList); with function-level x/y both share registers.
- `if (layout == 1) u->scr = IdSub.unitPtr(0xFD, 0x1E)->scr; else u->scr = IdSub.unitPtr(0xFB, 0x1E)->scr;`
  (the struct copy repeated in both arms) is what lets jump2 merge the two call tails and PRE the
  `lis r28, IdSub@ha` of both arms to the function top (MessageDisplay::init); a ternary index gives
  one call with a `clrlwi`, plain arms leave `li r5; bl` unmerged (the `use` after the call).
- `S16Set(x, ...)` for the member stores before a `pSys->language` read keeps the `lwz pSys` below
  the `sth`s; store order `state = 0; tplState = 0; tplFirst = 1` gives the target's
  `stb 0x12; stb 0x11; stb 0x10` (dying-first rule).
- `u8 page = fw->page` plus direct `fw->page` reads in the same ebb: the local becomes
  `lbz r8; clrlwi r27,r8,24` (the direct reads cse to the QI load pseudo, so `fw->page++` is
  `addi r0,r8,1`); with only the local in use the load is a plain `lbz` (MessageDisplay::move).
- `int* pReq = &file_tpl_req; sprintf(..); ...; *pReq = DVD_READ_N(..)` puts the `lis
  file_tpl_req@ha` in a callee-saved register before the sprintf; `x = call()` expands the call first
  (expr.c expand_assignment CALL_EXPR case) and loads the high part after it.
- ss_file's `.data` is 4 bytes short of the split object: `asm(".section .data; .balign 8")` at the end
  (the next unit's `.data` starts 8-aligned); ss_main the same.
- ss_main: `switch (info.type)` (not if/else-if) for the `cmpwi 1; beq; cmpwi 9; beq; b` chains;
  `&info`/`&size` recomputed per call through `static inline` wrappers (`ssItemInfo`, `ssReadCheck`);
  `PSet(wk->x240, wk->x23C)` keeps the following `lhz exam_id` below the store; `exam.move();
  exam.trans(); ... exam.quit()` (member calls, no local pointer) give the `mr r26,r30` PRE copy;
  the static const Vecs of `cLightInfo::init2` are function-local statics (emitted before the pool);
  numDisp takes `u8 id` (no `clrlwi` at the `unitPtr` calls) and copies `col0[0..3]` byte by byte
  (a struct copy is `lwz/stw`); weaponChangeRequest is `if (x4FB8 == 1) return; switch (x4FB8)
  {case 0: case 2..5:}` (the `cmpwi 1; beqlr` is a separate if); weaponChangeMoveCheck is
  `x250 != 3 && x250 != 4` (`subfic/subfe/neg`); `BitOn(ssPlModel->be_flag, 2)` reloads the model
  pointer for the following `alpha` store; the character switch order is Leon, Ashley, Ada,
  Krauser(4), HUNK(3), Wesker (as playerModelInit). sscrnCameraInit needs `const f32 zero = 0.0f`
  for the pool order (0.0 first).
- SubScreenTask: `SsTermMain* termMain = 0` and the other three null widget pointers are declared
  BEFORE `exitInit = new SsExitInit` (their `li`s are scheduled around the `__builtin_new` call and the
  first ctor's `i = 0` cse's to the first of them), `cur = 0` after `exitInit->connect`, and every
  branch calls `cur->init(wk)` itself (jump2 merges the call tails into the `& 0x40` branch's copy;
  the final else copy stays because of the `use` after the call). The model loops call through a
  function-pointer local (`void (*func)(cModel*) = sscrnModelTrans; for (m = MapMgr.pAlive; ...)
  func(m)` -> `mtlr r31; blrl`). `MotionMoveF(m, 0)` (pl_npc.cpp alias) for the `li r4,0`.
- SOLVED (ss_main sscrnCameraInit): the source order is `pos.z, up.y, at.x, at.y, at.z, pos.x,
  pos.y, up.x, up.z, fovy` — the LAST zero store in the source (`up.z`) carries the zero register's
  death and is issued first among the zero stores (weight rule), the others follow in source order,
  and `fovy` written last has its pool load issued last so `up.z` slips in front of it.
- OPEN (ss_main SubScreenTask, 96%): global-alloc swaps `wk`/`exitInit` (r21/r20) and `cur`
  (r28/r27), ours hoists one `lis pG@ha` (r24) out of the while loop where the target keeps three
  separate `lis` (two PRE'd before the weapon switch, one in the digit block), `&MapMgr` is a hoisted
  pointer (`addi r23, r11, MapMgr@l`) in the target, and the `cur->init(wk)` arms: the target
  cross-jumps `mr r4,r21; lwz r9,0xc(r28)` of every arm into one tail (each arm keeps only
  `mr r28,X; b`), ours keeps the two insns per arm because the fall-through arm schedules them
  `lwz; mr` (24 bytes). The cManager<cMap>::log copy is handled by the linkonce rule above.
- ss_item (src/Sscrn/ss_item.cpp) idioms: cursor state is a 9-byte `ItemScreenWork` (sscrn.h) at
  SUB_SCREEN+0x304 (`col`, `idx[2]`, `sel[2]`, `comb[2]`); the debug item-make state is the tail of
  the 0x34C debug block, addressed as one struct (`SsItemMakeWork`, `addi rX, wk, 0x34c` +
  displacements 0x1C/0x20); `int item_wait[1]` one-element array (the `lis` in r11, SsFileInit
  idiom); font statics `s16 w[2] = {0, 0x12}; s16 h[2] = {0, 0x18}; s8 space[4] = {-1,..}` used as
  `[1]`/`[1]`/`[3]` through the `setFontSizeS` alias; `itemTexNo`'s local `u8 tbl[105]` template
  (the label's 0x6B is padding to the pool); the `.data` `const char*` table and the two `int`s of the
  item-make menu are defined right before it (strings after itemFrameSet's pool); `IdNumN.unitPtrN
  (int, int)` / `IdNum.setI` / `numDispI` int views where the target has no `clrlwi`; a hidden `case
  0: break;` in itemFrameMove's state switch; `off = 0; if (!(flags & 1)) off = 1; if (off) HIDE
  else SHOW` with `goto HIDE` from the other condition (the `li 0; xori; andi.; beq; li 1; cmpwi`
  chain, HIDE laid out first); `d = old - iw->idx[col]` read back right after the store (forwarded
  register + `extsb`, the -1 store after it); `int no = i + 1` inside the unitPtr loop (`mr r31,r30`
  increment); `JOY* joy = &Joy[0]` local in itemMakeMove; `PSet((void*&) wk->x248, item_sel)`
  reloads `item_sel` for the following compare; ItemCommand::move: block-local loop counters per
  `dir |= 0xF` loop (r8, not a callee-saved register), the mode switch written `case 2, 0, 1, 3`
  (layout order), `!(mode > 2)` / `!(mode < 1)` nested (no range fold). OPEN: ITEM_PTR's out-of-range
  `return &item_dummy` is a fresh `lis/addi` in the target while the 0xFF return reuses the flags
  store's address register (ours cross-jumps both); SsItemMain::init `cur = sel; itemCameraInit(wk,
  &pG->Cam)` load order and IdNum `lis` register; itemFrameSet/itemSelect/itemMakeMove/itemMakeDisp
  register allocation; itemMakeInit's `(u16) types` ternary (`clrlwi 16` of the int) and the match
  loop's inline `>> 8`/`& 0xFF` compare order; ItemCommand::move's `cmpwi 1; blt`.
- The map model globals are named `ssPlModel`/`ssWepModel` (.bss 0x494/0x498, MapMgr works 0/1),
  `ssPlMotion`/`ssWepModel2` (.data 0x978/0x97C), renamed by hand in symbols.txt/sym_map.tsv
  (data labels have no .sym name for the sync tool); the generator attributes them to ss_map.cpp.
- ss_term (src/Sscrn/ss_term.cpp, the codec call screen; 29/29 named functions byte-identical,
  .data identical, NOT Matching: see the eof item below). Includes in .rodata order: light.h,
  event.h, map_obj.h, widget.h, then `dbg_button.h` — which is now the REAL header (cDbgButtonBase
  / cDbgWindowBase / cDbgButton with their inline virtuals; the 12 menu strings keep their parse
  order, and event.cpp / sscrn.cpp / ss_main stay byte-identical because nothing there constructs
  one). cDbgWindow (the 128-button debug window, key function LocalUpdate) is declared in
  ss_term.cpp *after* MakeCol/DbgDrawBoxFill (its "AddButton(): new failed." string follows
  MakeCol's pool), and SsTermInit/SsTermMain after it — so ss_term includes ss_main.h mid-file,
  after cDbgWindow. Widget<SUB_SCREEN> must be completed before dbg_button.h (a `static inline`
  reading `w->num` at the top) so its vtable comes out last: the .rodata vtable order is the
  reverse declaration order SsTermMain, SsTermInit, cDbgWindow, cDbgButton, cDbgWindowBase,
  cDbgButtonBase, Widget.
  Idioms: `col += (u8)(a * 255.0f) << 24; ...` (MakeCol: `+` chains stay `add`, a single
  expression turns the last one into `or`); cDbgButtonBase's x/y and cDbgWindowBase's x/y are
  `u32` (the LocalDisp int->float conversions have no `xoris`); the box call needs
  `f32 px/py/pw` conversions first, then `f32 ph = 14.0f; f32 bd = 2.0f;` locals and
  `DbgDrawBoxFill(px - bd, py - bd, pw + 0.0f, ph + bd, ...)` (pool 2^52, 14, 2, 0, 0.7, 0.3);
  `(y + 1) + b->y` needs an inline `dbgWindowRow(y)` (fold reassociates the literal otherwise);
  cFileList::init's zero stores are `text, list, cursor, pattern, filter` and `dir(d, f)` passes two
  uninitialised locals (no arg moves); `p = text; num = 0;` (not `num = 0; p = text`) keeps the
  fresh `li r0,0` after a strchr loop whose exit register cse would reuse; `p += strlen(p); p += 2`;
  `if (top + rows > num) end = num; else end = top + rows;` gives the `mr r28,r0` copy; the second
  template array (`char defFilter[12]`) is declared mid-block after the first alloc/strcpy.
  OpeMesTblInit reads the op archive through an inline `opArc(wk)` (pointer reloaded per statement
  because the table stores may alias, and `ofs + (u32) arc` is not reassociated with the +0x400);
  the un-rotated `for (;;) { if (!(s->time > cnt)) { if (!OpeSeqMove(s)) return 1; } else break; }`
  keeps the test at the loop top; `MessageControl* m = &cMes; int i = 0;` declared AFTER the
  preceding call keep `lis cMes` / `li i` below the `bl` (three Delete loops); the `state++` after
  `sscrnMainMenuInit` is `IntSet(x10, 0)` so the following `pSys` load stays below it; the frame
  needs `Vec pos; Vec ang = {0,0,0}; pos = term_pl_pos;` (pos slot first, memset second);
  `modelOn = 0; ended = 0;` come out reversed; `wk->pzzlOfs + (u32) wk->pBuf` (offset first).
  cFileList has an empty ctor and dtor (the empty static init pair and `global constructors
  keyed to MakeCol`), and the file-scope instance is the unreferenced 0x18 of .bss after
  term_read_req.
- OPEN (ss_term eof, keeps the unit off MATCHING): the original writes the vtables of cDbgButton
  and cDbgButtonBase (interface-unknown classes: only inline virtuals) and outputs `~cDbgButton`
  first among the eof functions, `~cDbgButtonBase` after `~Widget`; ours writes neither vtable
  (nothing references them: `AddButton` is never emitted) — .rodata is 0x30 short and the two
  dtors are missing. finish_vtable_vardecl writes an unknown-interface vtable only when its symbol
  was referenced, so the original had a reference our source does not create (or its later SN
  build writes every completed vtable in round 1, which would also explain the exact vtable order).
  `#pragma implementation "dbg_button.h"` makes ours write them but reorders the placed linkonce
  block; not pursued. Also open: terminalCameraInit's pool has four extra floats after the 1.333
  aspect (0.5, 3.1415927, 180, 240) that no instruction loads — mark_constant_pool drops
  unreferenced entries in our build, and `const f32` locals / `if (0)` code / unused inlines all
  emit nothing here (tested).
  Second pass (2026-09) findings: (a) `_._10cDbgButton` stores `_vt.14cDbgButtonBase` (0x3690,
  the base vtable before the inlined `delete name`), nothing in the REL references
  `_vt.10cDbgButton` (0x3630) — the referencing code was an emitted-then-dropped linkonce copy
  (the REL rule drops unreferenced first copies): the original build outputs a *used* but
  unreferenced comdat function (the implicit `cDbgButton::cDbgButton()` that the parsed
  `AddButton`'s `new cDbgButton` marks used, and likewise `Widget<SUB_SCREEN>::Widget(int)`), our
  cc1plus outputs comdat inlines only when `TREE_SYMBOL_REFERENCED` (decl2.c finish_file "stop
  lying" loop) — a compiler-build difference. A dropped stand-in that constructs a cDbgButton
  (`__attribute__((section(".gnu.linkonce.t.X")))` + modules.py LINKONCE_DROP) does write
  `_vt.10cDbgButton` in round 1, `~cDbgButton` right after the static-init function and
  `~cDbgButtonBase` in round 2 (tested), but the target's eof order also needs `~Widget` output
  in round 1 with `init`/`move` in round 2 (i.e. `_._t6Widget1Z10SUB_SCREEN` referenced by
  pre-finish_file code while `_vt.t6Widget` is not), which no source construct gives here
  (a direct dtor call inlines; an explicit ctor instantiation writes the vtable in round 1 and
  pulls init/move forward). Not applied. (b) The four dead floats are the pool of dead code
  (`tan(fovy * 0.5f * PI / 180.0f)`, `/ 240.0f` — the screenPos2puzzlePos formula) that the
  original kept: its varasm does not run mark_constant_pool (ours does, varasm.c
  output_constant_pool). The only zero-code reproduction is a file-scope
  `asm(".section .rodata; .long 0x3f000000,0x40490fdb,0x43340000,0x43700000; .text")` right after
  terminalCameraInit (compiler-build difference candidate #10; not applied since the unit cannot
  flip anyway). (c) partnerType's table had 12 leading zeros; the original has 14 (`{0 x14, 1 x5,
  2 x4, 3}`, .rodata 0x2a8..0x308) — fixed.
- ss_model (src/Sscrn/ss_model.cpp, the character and weapon model builders; 40/47 byte-identical,
  .rodata and .data identical, .text +12): include order map_obj.h, light.h, widget.h, atari.h;
  `PL_ARC(n)` = `PL_ARC_PTR(pG->pPlArc, n)` re-read per call (pG reloaded); the model archive at
  SUB_SCREEN::x210 is an `SsArc`; `ssModelAdd(m, bin, tpl)` = `m->addModel(ssModInfoMgr.create(bin,
  tpl))` (cSsModInfoMgr got an asm-labelled `create__11cModInfoMgrPvT1`); the light set is one
  `static inline ssModelLight(m)` whose two `static const Vec` are the single .rodata copy after
  weaponFilename's strings; per-character `static Vec pos/rot; static f32 scale` locals land in
  .data at their function (`SS_MODEL_PLACE` stores pos, rot, then scale z, y, x); weapon hang =
  `wep->pParts->pParent = m->getPartsPtr(10)` + stores written per field (an inline taking f32
  parameters hoists the constants across the call); the `scale = 0.5` half-scale switch needs
  `case 0x13: case 0x16: case 0x17: break;` labels (they root the tree at 0x17 and let the compare
  be shared with the later switches through `mfcr`/`mtcrf`) and a `cModel* p = wep->pParts` local
  for the three stores; wep11 is two switches (`case 0: default:` / range pairs); wep01-04/06 are
  `if (type == 0) .. else if (type == 1)`, wep10 `if (0) .. ; if (1) .. else ..`, wep13's colour
  bytes are stored in index order and its `pParent` store is a `PSet` (the pG load must stay
  below it); the unit ends with an unreferenced `static int = 0` (.data 0xA40). playerModelInit
  passes the u8 weapon number/type through int-parameter aliases (COMPILER-DIFF 4). Residual: the
  six character inits load the scale static's `lis` early into a callee-saved register (ours
  right before the `lfs`; chain / local / order variants tried) and wep09Init's two modelInit arms
  are cross-jumped in the original (compiler-build difference 6). Second pass (2026-09): the
  `lis` position is a consequence of the scale `lfs` staying BELOW the `m->rot = rot` word-copy
  stores in the original (the load-of-a-global-above-a-member-store rule, "Matching rules" above)
  while ours floats it above them (fixed scalar vs varying struct: no alias) so the `lis` stays
  adjacent; a `static f32 x[1]` array makes the load in-struct (ordered after the stores, `lis`
  hoisted into r29) but then every `m->scale.? = s` store re-reads it (3 `lfs`), and a `f32 sc =
  x[0]` local moves the load above the copies. No FSet-style lever exists for the block copy.

### Open

- t_emlist.cpp (0x6174 of code: a 0x3E0 work block behind a struct-member pointer reloaded after every
  store, 122 `const char*` name tables and a
  64-entry `{char name[16]; const char** flag, *type, *set, *x}` id table, TOOL_MENU-like char[]
  menus) has 36 of 53 functions matched (skeleton, data and menus done; the disp/camera/target functions
  are left); the stage rooms are split (config/G4BE08/modules.py); r10d, r10e, r11a (st1_2), r109, r107,
  r10a (st1_1) and r102 (st1_1 + st1_3) are Matching, r108 (st1_1/st1_3) is written with 9/15 functions
  exact; r11d (st1_3, 16/21 incl. reloc-only), r10f (st1_3, 11/14), r11e (st1_3, 16/18) and r119
  (st1_2, 26/27: only Init's table-address registers differ) have full sources (include/obj00.h,
  obj13.h, objGondola.h are their room-side views of the DOL objects); r10c (st1_2, 15/23,
  .rodata/.data equal) and r11b (st1_2, 8/14 + the nameless cLight block, .rodata equal) are written;
  st4_0 r410, r40b, r411 and st2_3 r22b, r229 are Matching, r40a (8/9) and r22a (6/7) written;
  r100 (st1_0) is Matching, r117 (st1_3, 20/21) and r11c (st1_3, 19/23 + reloc-name-only) are written
  (see "Stage rooms, st1 r100/r117/r11c pass"); the other st2/st4 rooms are unwritten (cSceObj.cpp,
  which r40c/r406/r40e/r220/r225 need, has no source yet);
  em_wrap.cpp matches in st1_0/st2_4 (Matching) and is one register-allocation diff away elsewhere.
- db_light.cpp (src/tools/db_light.cpp: the light editor, 0x12408 of code in t_camera/t_light/t_event;
  Tools = the same object with `SetToolLight` in front (`tools/db_light_tools.cpp`, DB_LIGHT_SET_TOOL_LIGHT),
  t_esp = Tools + `cLightTool::setLogMode` (`tools/db_light_esp.cpp`, DB_LIGHT_SET_LOG_MODE); every
  other function of those three objects is byte-identical to t_camera's) is fully written: 128 of
  134 functions byte-identical modulo relocs (Tools 129/135, t_esp 129/136; .rodata/.data/.bss
  byte-equal), see the db_light idioms at the end of this file. Remaining: printEditTable (-8:
  the flag-column `x*8` constants are cprop-folded in ours, the target keeps `x` a variable after
  the first `"P"/"-"` diamond and has two surviving `mr rX,y` giv copies), editColor (-0x10: the
  `tmp`/`c` 4-byte slot order — a by-value GXColor helper puts a 4-byte expansion-time temp first but
  one per inlined call), draw_light_graph (14 words: the `%1.6f` eprintf's `li r5,0` is scheduled
  2nd in the target and `col` gets r5 there), edit_cutsel (-4, the reverse #2/#4 `u8 line` case),
  edit_light_id_shadow (3 words, #2 mask one call later), edit_light_parent (r8/r10 for `n`, the
  case-2 `(id >> 16) + 101` temp untied). Not MATCHING anywhere yet.
  t_sce / t_movie (`tools/db_light_v2.cpp`, 0x1143C, unwritten): 120 of t_camera's functions are
  byte-identical there too, but the object has no cLightTool ctor/dtor/move/lightAnalysis/getCutNo,
  no cLitPathTool ctor/dtor/expand and no cVarRange/cVarLoop members except limitUpper/limitLower
  (the vtable is still there), starts with SetToolLight and its .bss ends with a 0x1C object at 0xA8
  instead of pLightEnv: an older build whose tool object lives elsewhere.
- The `.drs` archives are not rebuilt by `ninja` (`tools/drs.py rebuild` does one at a time); the
  sound bank's record types 1/2 and the p0/p1 parameters are not interpreted.
- Unit boundaries inside the big modules (Tools has ~39 source files) are not known; the `.gnu.linkonce`
  orphan sections of Sscrn/t_esp/t_event/t_id/t_movie/Tools/t_sce (their original ELFs had 12 extra
  sections between .text and .ctors, concatenated behind .text in the REL).

- em10 OPEN (3 left, pass-order class): `em10LostHead` (59: `a == 3` kept in cr4 across calls with compares at
  the dispatch and after `EmSetDie`; `if (X == K) { asm volatile("" :: "r"(X)); }` forces a cr4 compare at
  that site but pays a `bne cr4` per site and re-emits on every path -- not clean), `em10FindCk` (20: bell
  arms merged before sched1 in the original, in jump2 for us), `setHand` (5: `lwz tpl` before the compare).
  Five passes; do not re-attempt without a new mechanism.

## Don'ts

- Never change the semantics of a shared tool (strip_unused.py, fold_linkonce.py, sync_symbols.py,
  ngccc.py) to fit one unit. Make the new behaviour conditional on the evidence that distinguishes
  your case, and re-run the full `ninja` + DOL check before and after.

- Never run `git stash`, `git checkout -- <file>`, `git reset` or anything else that rewrites the shared
  working tree: other agents are editing it at the same time.

- Never edit `build/`, `build.ninja`, `objdiff.json`, or `config/G4BE08/splits.txt` by hand. The same goes
  for `config/G4BE08/modules/<mod>/splits.txt`: change unit boundaries in `config/G4BE08/modules.py` and
  re-run `tools/gen_rel_config.py`.
- Do not run interactive `objdiff-cli diff`; use `tools/fdiff.py` (one-shot).
- Do not commit; the orchestrator commits.

- Loop rotation is decided by stmt.c `expand_end_loop`'s scan for a jump to the loop end within the
  first ~30 insns: `while (1)` + a deep `break` stays un-rotated (test at top, `b top` at bottom);
  `if (!c) {...} else break;` stays un-rotated; `do { if (call()==1) break; SceSleep(1); } while (1);`
  gives the un-rotated poll; `for (;;) { if (c) break; }` and `while (c)` get rotated + duplicated test.
- Named struct arrays with non-constant initializers get a `memset` per row (C++ TYPE_FIELDS includes
  the class-name TYPE_DECL); the original used plain `void* tbl[N][M]`.
- A cEm local (0x3E0) against em.h's 0xDE0 cEm: `struct { u8 buf[0x3E0]; }` + `cEmConstruct asm("__3cEm")`
  + qualified `((cUnit*)&em)->cUnit::~cUnit()` reproduces frame, inlined dtor and the linkonce copies.
- After `sync_rel_symbols.py` the module split objects are not re-split by ninja; delete
  `build/G4BE08/config.json` to force it, otherwise unit_info/fdiff report stale names.
- COMPILER-DIFF candidate #7: the original duplicates the leading insns of a two-predecessor loop-test
  block into both predecessors (pool load / `lfs; fadds; fcmpu; stfs`), leaving the constant in a
  caller-saved f13 reloaded after the call; our gcse only inserts with partial availability.
- Room idioms found on r11d / r10f / r11e / r119 (src/st1/, 2026-09):
  - `RsfSet`/`RsfClear` store through a cast-then-deref word (flag_rsf.h `RsfFlagWord`, not
    `MEM_IN_STRUCT_P`): the rooms reload `pG` and their static work pointer *after* an RsfSet
    (`lwz r0,4(r3); oris; stw; lwz r9,pG`), which only a store that may alias fixed scalars gives. It
    does NOT explain the pool `lfs f1, 0.0` issued after the RsfSet store (r108/r118/r11d
    execShowView: `RTX_UNCHANGING_P` loads never depend on stores) - still OPEN.
  - Poll loops: `while (f() != 1) SceSleep(1);` = `b test; body; test: bl; cmpwi; bne` (r11d
    checkIronDoorKeyUse); the SceSleep-first door swing (`SndCall; L: SceSleep; rot -= spd; ...;
    if (!(rot < lim)) goto L`) is `goto open; wait: SceSleep(1); open: ...` like r113 (constants
    reloaded per iteration because a goto loop has no loop notes).
  - `int eff = EspPullCoreKind();` with `(u8) eff` at every use gives the `clrlwi r10,r23,24` at
    EstSet and one PRE'd `clrlwi r30,r23,24` before the three Effect*Delete calls (r10f DoorOpen); a
    `u8 eff` local masks nothing.
  - Template-copied locals (`int list[11] = {...}`, `Vec pos = {..}`) whose copies the target issues
    *after* a run of calls are declared mid-block after those calls (C++), and a `cEmWrap em;` whose
    ctor `bl` follows them is declared after them too (r11d execEmAppear_end, r10f DoorOpen).
  - COMPILER-DIFF #4 in the rooms: `setPtr(s16,..)`/`setEm(s16,..)` called with `int list[i]`
    elements get `lwz` straight into r4 in the original (ours `lhz; extsh`): int-parameter aliases
    `cEmWrapSetPtrI asm("setPtr__7cEmWrapsSci")`, `setEmI asm("setEm__FsSciii")` (r11d).
  - `for (i = 0; i < 11; i++) f(list[i])` over a local array: pointer compare `cmplw r31,r28; ble`
    needs a `u32 i` (`int i` gives `cmpw`); ours still initialises the loop pointer straight from the
    template-copy register where the original keeps an extra `mr r4,r8`/`mr r31,r4` copy (OPEN).
  - A work pointer whose element stores are followed by a reload of both the pointer and the element
    (`stw r3,0(r9); lwz r11,work; lwzx r3,r11,r29`) is the one-member-struct global (r10f
    `R10fWorkPtr r10f_work; r10f_work.p->gondola[i] = ...`); a typed `PSet(cObjGondola*&, ..)` gives
    `stwx r3,r29,r9` (index first) instead. Single pointer fields keep the typed `PSet` (r11d `mi`,
    r11e `rock[i]`, r119 `dog`).
  - Two accessor results stored through one local (`cLight* l; l = LightMgr.getWorkPtr(2); l->power
    = a; l = LightMgr.getWorkPtr(6); l->power = b;`) share one register (r10 twice); separate
    expressions get r10/r11 (r119 ThunderFlagOn/Off).
  - Repeated scroll-object blocks in the Evt_*_Func handlers (`w = SmdGetWorkPtr(id); if ((obj =
    SmdGetObjPtr(id)) && w) { setPos(&w->pos); setAng(&w->rot); }` x4) are written out with two
    function-scope locals (`cObj* obj` before `SmdWork* w`: obj r31, w r30); an inline helper with
    its own locals swaps the registers. The `SetMod(name, obj, 5, 0, 2, 0); setPos; setAng; be_flag
    |= 0x20; EspSetModelPtr` blocks are written out too: a helper taking the name string evaluates
    the string address before the `SmdGetObjPtr` call (r119 Evt_R119S00/S20_Func).
  - `pG->flags_60 >= 0` on the u32 field must be written `(int) pG->flags_60 >= 0` (`cmpwi; blt`);
    the unsigned form folds to true and the whole test disappears.
  - A call result tested and used in one block (`mr. r3,r3` after `SmdGetObjPtr`) is a block-local
    variable; a function-scope `cObj* obj` also used in other blocks gets `mr. r31,r3`.
  - `switch (e->funcMode)` with `cmpwi 1; beq; ble end; cmpwi 2; beq` has an empty `case 0: break;`.
  - `if (cnt > 899) for (o = ObjMgr.pAlive; o; o = o->next)` is the plain rotated loop; an explicit
    `&& ObjMgr.pAlive != 0` in the `if` adds a `mr r9,r0` copy of the head.
  - Routine bytes `xFC..xFF = 0` written in that source order are issued `ff, fc, fd, fe` (r11e
    funcAshley).
  - OPEN (r11d checkEmReset): `for (;;) { while (count > 10) SceSleep(1); setEm(tbl[i]); i++; if
    (i == 10) break; SceSleep(60); SceSleep(1); }` - the original keeps `cmpwi r31,9; li r3,0x3c;
    addi r31,1; bne` with SceSleep(60) laid out before the inner loop body; ours hoists the `i+1`
    into the outer loop header (interblock scheduling) and never cross-jumps the trailing
    `SceSleep(1)` into the inner loop body. goto / do-while / `i++ == 9` forms tried.
  - OPEN (r11e / r119 Init): the `lis` pseudos of the pos/rot table addresses passed to six
    `SatMgr/EatMgr.create` calls get callee-saved registers pair-wise (rot above pos in the original,
    pos above rot in ours for some pairs); arrays, eight separate statics, pointer locals and a dozen
    symbol names tried (not a name-hash effect).
  - OPEN (r10f GondolaGetOn/GetOff): `&posA[side]` is formed off the `mot` table's frame pseudo
    (`add r4,r26,r23; addi r4,r4,0x18`) and `mulli r26,r22,0xc` is issued before the SndStrReq call
    in the original; r11d appearLittleSister: a `void* zero` local's `li r31,0` survives next to the
    `andis.` result that cse merges it with in ours.
- Room idioms found on r10c / r11b / r229 / r22a (st1_2, st2_3) and r410 / r40b / r411 / r40a
  (st4_0; r410, r40b, r411, r22b, r229 Matching, 2026-09):
  - Work pointer reloaded after a store *through* it (`stw r3,0x74(r9); lwz r9,work; lwz 0x70(r9)`,
    `sth hp; lwz work` chains) = the one-member struct global (`R10cWorkPtr r10c_work; .p->`) as in
    r10f; stores into the work that are followed by a `pG`/`pPL`/`pSys` load in the original need a
    typed `PSet(cSat*&, ..)` / `U32Set(cnt, cnt + 1)` on top (a struct store lets ours hoist the
    fixed-scalar load above it). The calloc store whose `lis work@ha` sits in a callee-saved register
    *before* earlier calls is a reference variable `R11bWork*& wp = r11b_work.p;` declared at the
    top (`wp = MEM_CALLOC(..)`); ours then merges that `lis` with the PRE'd one of the later loads
    (r11b -4 bytes, OPEN), r229 (no later loads in Init) matches.
  - Two `EM_LIST(n)` byte stores with one `pG` load (`lbz flags; stb x3; ori; stb flags`) are written
    through a local `EmListData* l = EM_LIST(n);` (a QI store reloads `pG` otherwise), and an
    `EM_LIST(n)->x3 = 0` after an `EmSetFromList2(n, ..)` whose `pG + 0x5xx8` address is computed
    *before* the call is `EmListData* l = EM_LIST(n); em = EmSetFromList2(n, 1); l->x3 = 0;`.
  - `cPlayer* p = pPL; p->setPos(&v); p->setAng(&v)` when one `lwz pPL` feeds two calls (`mr r3,r30`
    twice); `pPLS->setPos(); pPLS->setAng()` (the struct view, twice, no local) when the original
    reloads pPL per call but keeps the first load below preceding Vec template stores (r10c ItemGet).
  - Frame-slot reuse decides block scoping: r10c/r22a's rope event has `{ Mtx m; ... FadeSetW(2,30);
    SceSleep(30); ObjMgr.destroy(obj); } { Vec pos2 = {..}; Vec ang2; f32 ry; Vec* pa = &ang2; ... }`
    in each arm — the else arm's FadeSetW colour pair lands at a fresh 0x68 slot because `m` is still
    live there, and `pos2` reuses 0x38 because `m` is dead; `Vec* pa = &ang2` declared at the block top
    puts the `addi r28,r1,0x48` into a callee-saved register before the FadeSet.
  - `if (RsfCheck(..) == 0) { SceSleep(1); } else break;` inside `for (;;)` is the un-rotated poll
    (`test; bne exit; sleep; b test`); `do { .. if (c) break; SceSleep(1); } while (1);` for the
    un-rotated mid-body break (r10c ItemGet), and `while (call() == 0) SceSleep(1)` for the
    `b test` form.
  - `dir ? (a < lim) : (a > lim)` as an `if` condition gives the `blt L; b L2; L34: ble L2` pair of
    arms (r10c SwitchExec); `if (n == 0) return; if (n == 1) return;` keeps two `beq`s where
    `n != 0 && n != 1` / a switch range-folds (r40a em_set).
  - `int skip = 1; if ((e->status & 0x40000000) == 0) skip = 0;` in a `static inline` (`li 1; andis.;
    bne; li 0; cmpwi`) — the `? 1 : 0` ternary on a single bit folds to `extrwi`; an inline helper
    with `void*& mod` keeps one GetMod slot for every event cut (r11b Evt_R11BS00_Func).
  - `cObj* obj = 0;` at the top *used* as an EstSet argument is the SI zero pseudo set in the first
    block (`li r27,0` before the first branch) that later byte stores (`l->x3 = 0`) and stack args
    share; unused it is deleted and the zero is created at the EstSet (r11b Init).
  - `if (spdY < -100.0f) spdY *= 0.8f; else spdY *= 0.935f;` gives the cross-jumped `fmuls` after the
    two `lfs f0` arms; `spdY *= k` with a `k` local fuses into the following `fmadds`. A member load
    reused across blocks (`dy = obj->pos.y - lim; ... obj->pos.y < lim`) must be written twice
    (gcse PRE copy `fmr f11,f0`), not cached in a local (r10c hako_down).
  - A block of `for (i = 0; i < 80; i++) { SceSleep(1); .. }` with a constant start has no entry
    test (`cmplwi 0x4f; ble` with `u32 i`); `f32 spd = 0.0f; f32 max = 100.0f;` declared mid-block
    (after the `BitOn(obj->be_flag, 0x20)`) keep their `lfs` below the preceding calls (r22a EleDown).
  - `FSet`/`FAdd`/`FSub` on `SmdGetObjPtr(id)->pos.y` where the original reloads `pPL` after the
    store; `pPL->pos.y = K` stores that reload pPL between each other are `FSetP(pPL->pos.y, K)`
    (r22a), `wheel->rotSpd.z` stores followed by a `pG` load are `FSet` too (r10c moveWheel).
  - A room whose `.rodata` carries `"event/evd/rNNNsXX.evd"` / `"evt_.._func"` strings with no code
    using them had a never-called static function (r229 `r229_evtSetup`): the original REL link did
    strip room objects at function level too; add the unit to modules.py `STRIP_UNUSED`.
  - Unit boundary: st2_3 r22b's group starts with the HALT string that r22a's pin had swallowed
    (0x2A88 -> 0x2A78); always check the last words of the previous room's `.rodata`.
  - TexRender rooms (r10c, r229, r11b): `u8* tbl = r10c_texTbl;` local for the blend table (`addi r25`
    kept, `stb 0xf7,4(r25)`), `tex->sy = tex->sx = 0x40` chain, per-object `x136 = 2; x137 = 0x12;
    x138 = 0xA0;` written in that order for every object (the scheduler emits 138,136,137 or
    138,137,136 per block by itself); `TexRenderModRes(cModel*)` reads a parts number from r4:
    `void TexRenderModResP(cModel*, int) asm("TexRenderModRes")`; `ModelInfoRefrectOn` is C++
    (model.h). `u8 GetEmIdFromList()` passed on unmasked: `int GetEmIdFromListI(u32) asm(..)`.
  - OPEN: independent `stfs` of two pool constants into a Vec (`v.x = 3145; v.z = 10394; v.y = 0`)
    come out x-first in the original and z-first in ours whatever the statement order (r10c
    EmEvent/EmEvent_exit, r40a first_init: the FPR pair f0/f13 swaps with them); the second word
    pair of a `Vec = {..}` template copy is loaded 8-then-4 in ours (r10c/r22a rope event; `static
    const Vec` sources fix the stores but not the loads); `-100.0f` in a nested `if` inside a large
    loop is hoisted by our second loop pass and not by the original (r10c hako_down);
    r10c SetEmHitAtari's 0.01/0.05/0.06 initialisers sit in the pool between the first if's
    Yarare constants and its else constants while their loads precede the first RsfCheck;
    r11b EmSetChange's `stb x3 = 0` is issued last by the original and second by ours.

- Locals whose frame slot sits inside freed inline-table slots must be declared after the getter
  calls: `assign_stack_temp` best-fits into the merged freed region; only fresh allocations extend the
  frame (trans `ShadowCastSetup`/`SelfShadowSetup`).
- A load the original does not hoist above scalar-global stores means the stores were not
  `MEM_SCALAR_P`: write the increments as `ISet(g, g + 1)` (an `INDIRECT_REF` of a plain pointer sets
  neither IN_STRUCT nor SCALAR, so `true_dependence` keeps the order).
- Branch arms ending in a call block cross-jumping of a shared tail (flow.c appends
  `(use (const_int 0))` after a block-ending CALL_INSN); duplicate the tail through a non-call
  statement into each arm to get the original's merged `li r7; bl`.

- A ctor that stores a base field before the vptr store has it in a base-class initializer
  (`Event::Event(u8) : cUnit(1)` -> `cUnit(u32 flag)` overload).
- A `u32` passed to a `u8` ctor parameter with no `clrlwi`: declare an int-parameter alias with
  `asm("__5EventUc")` and call it (GNU v2 ctors return `this`).
- `pWork[i].field` written at every use (no element pointer local) reloads pWork after each store.
- `if (ok) { body; return 1; } err; return 0;` places the err block at the end and cross-jumps it.
- `int n = 37; for (i = 0; i < n; ...)` gives `blt end`; a literal bound folds to `i <= 36`/`ble`.
- Prototyped `memset(p,0,12)` is a plain call; a zero aggregate initializer is the `crclr`+`memset`
  libcall -- both coexist in one TU.
- Address-taken scalars declared before a `char buf[]` get frame slots after the array unless the
  array is declared in an inner block after the first `&scalar` use.
- `Obj18Work* w = &obj->o18` produces `addi r11,r9,0x328; lwz 0x68(r11)` instead of a folded offset.
- A single-variable fade helper (`c = 0xFF; start = c; c = 0; end = c;`) delays the `li r0,0` to just
  before its `stw`.

- A `const T x[] = {..}` declared `extern` in a header is emitted at its definition; with internal
  linkage it is deferred behind the cManager template strings (cam_ctrl `smooth_ratio`).
- Empty in-class `C() {}`/`~C() {}` on a vtable-less class emits no body but makes its global object
  emit at the definition point (controls `.bss` order vs deferred plain arrays).
- `char st = member; switch (st) { ... member = st + 1; }` reuses the loaded byte for the increment.
- `(on & A) || (on & B)` on one lvalue folds to one mask; separate `andi.` tests need inline helpers.
- A do-while `{}` macro flips FPR assignment of two independent RMW chains vs a plain block.
- Constant folding needs the exact product literal (`0.8f * 1.2f`, `PI * 0.35f`, `1.33333333f`).
- A `lis rX,0x8023` with no reloc in the split object next to our `@ha` reloc is a dtk pairing miss.

- An expression computed identically at the end of both if/else arms is merged by jump2 into one insn
  placed before the join label, ahead of the join block's own loads.
- Frame size: total = ALIGN8(8 + ALIGN8(vars) + fpmem(8, +4 if `-(fp+gp)-8` not 8-aligned) + ALIGN8(fp+gp)).
- gcse PRE pseudo numbering can wrap (hash mod table size) and flip two giv registers; one extra
  pseudo before the copies (`int dead = 0;`, COMPILER-DIFF) fixes it. Register-priority ties are
  broken by qty/allocno number, not host qsort.
- A strength-reduced index passed to a call (`f(id - i)`) puts the giv init `li` after the hoisted
  invariants; a separate variable puts it before. A `do{}while(0)` macro around a loop inflates the
  loop-weighted refs of hoisted invariants.
- A float local assigned twice gets global alloc; one variable per value keeps all local-alloc'd
  (f31..f27 by declaration order). A loop counter shared by two loops is one low-priority pseudo.
- `no = g->x; switch (no) { case N: num = N; }` folds the case constant into the switch register.
- `bitTblChk((u32) used, i)` (integer table parameter of an inline) gives `lwzx` with the offset in a
  BASE_REGS register.
- Tools: fdiff.py writes a per-pid json (agents run concurrently); keep private helper scripts in
  your own /tmp subdirectory -- /tmp is shared and files were overwritten.

- db_light (tool module, 0x12408): every store through the tool pointer reloads it (a struct member,
  `LightToolPtr.p`); a group of u8 stores through it with *one* load is a chain
  (`p->a = p->b = p->c = 0`: all `p->` loads precede the stores, and the stores come out outer,
  innermost, ..., i.e. `x10 = x11 = x12 = x13 = 0` stores 10, 13, 12, 11). A `bool` is 4 bytes here.
- A pointer global whose load must stay *after* a store through an unrelated pointer type (pathSelect:
  `path[i] = NULL; createPath(pLitPath)`) is a struct member too (record alias set 0);
  a plain static is hoisted above the store.
- Menu loops: `eprintf(x, C + i * 14, ..., *name++)` (y a giv of i, the name a separate pointer biv)
  gives `li rY,C` last in the preheader and keeps `i` (`cmplwi i,N; ble`); a `y += 14` variable or
  `tbl[i]` indexing drops the counter for a pointer compare.
- `for (i = 0; p->data[i] <= 200; i++)` with an unsigned x step (`u32 xi += 4`) converts with the
  2^52 magic (no `xoris`); `(f32) (int) (u8) x` / `(f32) (s16) (u16) x` pick the signed conversion of
  a zero-extended byte / half (`lbz; xoris` / `lhz; sth; psq_l qr5`).
- `static int state = 0;` goes to .data (explicit zero init), `static f32 val;` to .bss; function-local
  statics are emitted in text order, file-scope statics after them in declaration order.
- `#line N "D:/Bio4/Prog/db_light.cpp"` before a `VECNormalize(...)` macro use reproduces its
  `__LINE__` (0xBA1, 0xC1B, 0xFF2).
- A `GXColor black = {0,0,0,0}` local is folded to `li 0`; the original loads the constant from .rodata
  once (r24) and copies it (`stw r24, 8(r1)`) into a 4-byte `GXColor tmp` local before each
  `DrawTile(&tmp)`: by-value GXColor helpers give 8-byte BLKmode temps instead.
- `GXSetChanAmbColor(0, whiteCol())` (inline returning the struct) plus `Mtx44 proj; Mtx mtx;`
  declared *after* the colour calls lays the frame out as arg temp 0x8, proj 0x10, mtx 0x50, colour
  0x80 (DrawTile).
- `if (a) { if (b == 0) return 0; return 1; } return 0;` puts `li r3,0` before each `beq` to the
  epilogue (SetToolLight); `if (!a) return 0; if (!b) return 0; return 1;` shares one `li r3,0` block.
- `cursor = (mode == 2)` is a setcc (`xori; subfic; adde`); `!= 2` (and every other spelling) branches.
- `pTool->cursor %= 0x100` on a u8 member gives `rlwinm r0,r0,0,23,23; subf` (the range-narrowed
  `% 256`).
- `u8 num = (a < 60) ? 60 : a;` keeps the `clrlwi` truncation at the join; `u8 num = a; if (num < 60)
  num = 60;` drops it.
- In a switch with `default:` written first, the default body precedes the case bodies and the case
  stores cross-jump into one `stb` (save's mode -> cursor).
- `sprintf(path, ...)` then `cDbLit lit;` (a mid-block declaration) constructs after the call.
- A switch on `pTool->cursor` inside `case 1:` whose cases all end in `pTool->x8 = 2/3/4; cursor = 0`
  is emitted as one shared tail (`stb r0,0x28; stb r11,0x6`) with `li` pairs per case (edit_tune).
- Things that did not move the needle (register / `lis @ha` pseudo choice): declaration order of
  locals, `int` vs `u32` counters, `xi = x` before the loop vs in the `for`, `by-value` vs `const&`
  GXColor helpers. The remaining db_light diffs are of that kind.
- db_light, second pass (121/134 in t_camera, 121/135 Tools, 122/136 t_esp; .rodata/.data now byte-equal):
  - Judge with a masked byte compare of the split vs compiled object (relocated fields masked on
    *both* sides, 16-bit relocs sit at word+2): objdiff's % hid the `xAxis` value (it is (0,1,0)), the
    editColor black template, `logX = 24.0f`, `sz = gamma2 = 0.25f`, `x1C = 3000.0f`, `* 0.1f` in move,
    string-space mistakes and the 0x400921FB60000000 double (`atan2(..) * 127.0 / 3.14159265f`, an f32
    PI widened).
  - "First-entry init" blocks (`if (pTool->x8 == 0) { x9 = cur->xD; x8 = 1; }`) have an else arm with
    a dead local set in both arms (`first = 1` / `first = 0`; one dead set is deleted by jump1, two
    survive to cse): the then arm ends in a jump, cse cannot skip it, and the join's `pTool` load
    re-uses the block's own `lis LightToolPtr@ha` instead of the hoisted one (select_type,
    shadow_select_type, edit_light_id, edit_light_select_sub, edit_light_parent).
  - Clamps are if/else with a store per arm (`if (cursor + 10 < 0xFF) cursor += 10; else cursor =
    0xFF;`): the stores are cross-jumped after reload and the join is a new cse ebb (fresh `lis`);
    `n = ..; if (n > 0xFE) n = 0xFF; store` is a skippable block that keeps the shared @ha register
    (edit_cutsel_main).
  - `cVarLoop::limitUpper` declares `range` before `v` (limitLower the other way round): the
    declaration order decides the load/compare schedule of the entry block.
  - A chain `l->x138 = pad[0] = pad[1] = pad[2] = 0` stores 138, 13b, 13a, 139 (initLightWork).
  - drawPath: the data pointer steps (`u8* d = p->data` declared with the locals, `*d`, `d++`) while
    `i` stays a counter; the `for` increments are written `xi += 4, i++, d++` (`addi xi` before
    `addi i`).
  - createLit: the same `cLightEnv* c` local in the table loop and the memcpy loop ties `c` to r4
    (the memcpy argument) in both.
  - cLitPathTool ctor: the else arm stores through a reference (`cLightPathHeader*& p = pLitPath; p =
    getPathHeader()`), which evaluates `lis LitPathPtr@ha` before the call into a callee-saved reg.
  - `static const` file-scope objects are output after the vtables in the original .rodata (finish_file
    order: vtables, then deferred namespace statics, in declaration order): the black GXColor template
    of editColor (0x1630) and the (0,1,0) axis (0x1634, an `f32[3]`: 4-aligned, a `Vec` would be 8).
    tools/ngccc.py now puts module `.gnu.linkonce.d.*` vtables in `.rodata` in place (like the DOL
    path) instead of fold_linkonce appending them; all 111 files still OK.
  - printEditTable's header table is a file-scope `static const char* table_head[]` defined *before*
    the inline row printer: an inline's string literals are emitted at parse time, a function-local
    static's initializer strings after the body.
  - editColor: `black` is a local copied from the deferred template (`GXColor black = blackTemplate;`
    right before the first use: one `lwz` into a callee-saved reg, `stw` into `tmp` per DrawTile); a
    `{0,0,0,0}` initializer folds to `li 0`. Open: the frame order tmp(0x8)/c(0xC) — both are
    ADDRESSOF pseudos and ours forces `c` first (`c.g = 0` is `(plus (addressof c) 1)`), the
    original forces `tmp` first while still storing c.r/c.g/c.b before the first DrawTile.
  - COMPILER-DIFF #2 in edit_light_id_shadow: the original zero-extends the u8 `col` once before the
    two uses of the else arm (`clrlwi r30,r30,24`, PRE-shared); reproduced with `int c = col; asm("" :
    "+r"(c)); (u8) c` except that our sched1 places the mask after the arm's first call (calls do
    not end sched blocks here). edit_cutsel's `line`: `u8 line = i + 6` gives the target's frame and
    size but a `clrlwi` where the original has a plain `mr` (the reverse #2/#4 case), so `int line`
    stays (-4 bytes).
  - Open register-only diffs: edit_light_type_spotlight/direct/parallel (the `spotRot`/`dirRot`
    `high` pseudo shares r30 with the `Vec* rot` pointer because the pointer's `addi` is scheduled
    after the `x = 0` store in the original and before it in ours: `lis r30; stfs @l(r30); addi
    r30,r30,@l`), drawLightInfo_SpotShadow (`mr r3/r4` of Draw_corn2 hoisted above the `len == 0`
    branch — COMPILER-DIFF #5), edit_light_parent (`n` r8 vs r10: the case-2 temp `(id >> 16) + 101`
    is not tied to `id >> 16`), lightAnalysis (+0x1C), move (+0x10), draw_light_graph (-0x34),
    printEditTable (-0xC, one more callee-saved register and a smaller frame in the original),
    edit_light_type_shadow_fit (-4).
- db_light, third pass (128/134 in t_camera; lightAnalysis, move, shadow_fit, drawLightInfo_SpotShadow,
  spotlight, direct, parallel now byte-identical):
  - `ObjMgrWork(i)` (obj.h) has the `no >= nArray` range check; lightAnalysis' scan loop has none:
    a local `objWorkNoChk(i)` (`pArray + size * no`). `if (objWorkNoChk(i)->isAlive()) { cObj* obj =
    objWorkNoChk(i); ...}` gives the target's `lwzx r0,r11,r9` (be_flag through the folded address)
    plus a separate `add r31,r11,r9` for the pointer; one `obj` local used for both gives `lwz 0(r31)`.
    move's bounding-box loop *keeps* the check (`blt L1; li r31,0; b L2`), written with a
    `cObjMgr* m = &ObjMgr` pointer inside the inline (`objWorkChkP`); the plain obj.h form lets
    thread_jumps + cse fold the check away (ours), the pointer form does not.
  - `addi r3,rObj,0x164; mr r29,r3` (an address computed into the argument register, then copied) is
    a gcse PRE copy: `obj->lightInfo.getLightNum()` called twice with an if/else *between* the two
    calls (the join label ends cse's ebb, so the second `&obj->lightInfo` is PRE'd: the precomputed
    argument pseudo dies at the `mr r3` and local-alloc gives it r3). A `cLightInfo* info` local
    gives `addi r30; mr r3,r30` — cse rewrites the later `&obj->lightInfo` into a copy of the arg
    pseudo even when it sits in a then-block (conditional jumps do not end cse's ebb; only labels do).
  - Two identical `0x150 + i*14`-style givs (`eprintf(x, 0x2A + i * 14, ...)` in *each* arm of an
    if/else inside the loop) are NOT combined by combine_givs when each is single-use (loop.c refuses
    to combine into a single-use DEST_REG giv): two `li rY,0x2a` in the preheader and two `addi 0xe`
    in the latch. A `y += 14` variable gives one.
  - A common address in both arms of a store diamond (`stb r3,3(r9)` / `stb r26,3(r9)` after ONE
    `lbz anaNum; lwz anaTbl; slwi; add`) is a pointer local computed before the `if`:
    `u8* p = (u8*) (anaNum * 4 + (u32) anaTbl);` (integer sum: index first in the `add`).
  - `int st = state; switch (st) { case 0: ...; case 1: color = st; }` reuses the switch register for
    the store (`stb r11,0x1e`); `color = state` reloads the member because the case label starts a new
    cse ebb. The `if (c) x = 0; else x = 0x14;` diamond order: `int c; if (modeSel == 0) c = 0; else
    c = 0x14; eprintf(.., c, ..)` gives the target's `li r5,0x14; beq; li r5,0` (jump.c's "x = b; if
    (...) x = a" hoists the *else* set when both arms are single constant sets and the if/else is a
    statement); the ternary `modeSel == 0 ? 0 : 0x14` as a call argument gives `li 0; bne; li 0x14`.
  - `gameCutNo = cutNo = getCutNo();` (dying-first rule: the store of the chain's last assignment is
    issued first, so the target's `stb 0x15; stb 0x14` order needs cutNo innermost).
  - `BitOff(pG->flags_170, ..)` (reference store) makes the following `pG->flags_60 &= ~..` reload
    `pG` from a PRE'd `high(pG)` register (`lis r28,pG@ha` inserted at every state-switch exit).
  - `pLog->x = (int) logX; pLog->y = (int) logY;` with one `lwz pLog` and no `lfs logY` reload: `int x
    = (int) logX; int y = (int) logY; cLog* l = pLog.p; l->x = x; l->y = y;` (the `sth` through
    pLog->p may alias `this->logY` and `pLog` itself).
  - draw_light_graph: the static graph parameters are `x0/y0/w0/h0/s0` — the static names decide
    gcse's `high(sym)` hash order and thus which of the two loop-hoisted address pseudos (gx/gy) gets
    r20/r21 (`gx/gy`, `x/y`(clash), `px..`, `graph_x..` all gave the swap). The distance and the
    1000-step marker share ONE variable `x` (`x = GetDistance3(..); if (x < l->x1C ..) { t = x / scale;
    ...}; func_attn(l, x); for (x = 1000.0f; ...)`) — that puts d in f30 and issues `fmr f1,f30` before
    `mr r3,r26` in the `%3.6f` call; `t = d / scale` is computed inside the then-arm only (the else arm
    stores the compare's 0.0 register as a.z/b.z). `col`: `v = func_attn(l, w0*scale); col = 0; if (v >
    0.04f) col = 6;` then `int c = col; asm("" : "+r"(c)); eprintf(.., (u8) c, ..)` (COMPILER-DIFF 2:
    the original masks the u8 argument; `col` must not cross the call so its `li` stays after the `bl`).
  - COMPILER-DIFF 2 with two uses (edit_light_type_shadow_fit): `int c; if (..) { eprintf(ON); c = 0;
    asm("" : "+r"(c)); } else { eprintf(OFF); c = 0x14; asm("" : "+r"(c)); }` then `(u8) c` at both
    eprintfs: the launder must sit in the ARMS — gcse's LCM only hoists/shares an expression whose
    operands are not set earlier in the same block as its first occurrence (`clrlwi r5,r5,24; mr r27,r5`
    at the join = the first extension into the arg register plus the PRE copy for the second use). With
    the asm in the join block (`int c = col; asm; (u8) c` twice) both uses extend separately.
  - COMPILER-DIFF 5 (drawLightInfo_SpotShadow): `Vec* pp = &pos; Vec* pn = &n; asm("" : "+r"(pp),
    "+r"(pn));` before the `if (len == 0.0f)` puts Draw_corn2's `mr r3,r25; mr r4,r26` above the
    branch (the laundered pseudos die at the call's arg copies and take r3/r4 in local-alloc).
  - `lis r30,spotRot@ha; stfs @l(r30); addi r30,r30,@l` (the `high` and the `Vec* rot` pointer share
    a register): compute `f32 sx = -(f32) pTool->joy.sx / 1000.0f;` BEFORE `Vec* rot = &spotRot;`
    (the psq_l/fneg/fdivs chain then follows the `x = 0` store and the `addi` is issued after it).
    spotlight's per-case `step` is ONE function-scope `f32 step` (cases 2 and 3 share f11); parallel's
    case 1 uses two variables (`c` for normal.x, `d` for normal.z) with the function-scope `const f32
    k` (one `f32 c` reused for both chains swaps f30/f31).
  - printEditTable: `y` is a giv (`0x150 + i * 14` written at each use: the `%02d` eprintf, the
    `printEditRow(l, 0x150 + i * 14, c)` argument, the EMPTY WORK eprintf) — its reduced register is
    `li r23,0x150` after the hoisted `lis` and the row printer's non-const `int y` parameter copy
    survives as `mr r29,r23` (the target has a second copy `mr r26,r23` used by the first "%s" column
    only — unexplained). `u8 c` (the colour goes through a temp: `li r5,..; mr r31,r5`), `col.a =
    l->color.r; col.r = col.b = col.g = l->color.r;` (two loads: a first, then g, b, r), and the swatch
    through a by-value helper (`drawColorTileC(int, int, int, int, GXColor c) { DrawTile(.., &c); }`: an
    address-taken by-value parameter gets a 4-byte SImode `assign_stack_temp` at the expansion, before
    the purge-time slot of the address-taken `col`, so the frame is temp 0x8 / col 0xC / fpmem 0x10).
    Each inlined call of such a helper allocates its OWN 4-byte temp (keep=1 slots are never freed), so
    editColor's nine DrawTiles cannot use it (frame +0x20).

### System units (file_app, pl_sub Matching; dvd, EtcModel, pl_class, mes, datactrl, read, objWep, lightPath, rnd, at_mod near)

- Masked-byte compare per function (aligned by symbol name) is the only reliable judge: dtk
  synthesizes `Sym+off` relocs and misses `@ha` pairings (`lis r3,0x802e` with no reloc), NgcAs emits
  REL14/REL24 for local branches, static locals carry `.NNN` suffixes, so a byte-identical function
  shows 92-99% in objdiff and a 4-byte trailing `.rodata` pad (dtk align 8) is normal (sn_malloc,
  hermite, quake, pl_event, snd_seq3 ... are Matching with `.rodata` 4 bytes short).
- `.rodata` string order proves the *parse* order of the unit: dvd's file table strings precede the
  map_obj.h/light.h/widget.h/card.h/sofdec.h strings, so `FileTbl[]` is defined before those
  headers are included (dvd.h first, table, then the rest). Two message strings that objdiff cannot
  tell apart were swapped (MRAM/ARAM Snddata).
- Cross-jump survivor mechanism (COMPILER-DIFF 6, jump.c read): `do_cross_jump (insn, ...)` deletes the
  tail of the jump being *scanned* and redirects it to the candidate's tail, candidates come from
  `jump_chain[label]` (prepended, i.e. latest first) for `b end` jumps, and from `jump_chain[0]` for
  RETURN insns, which is populated *during the scan* by the `b end` -> RETURN conversion in leaf
  functions. `find_cross_jump` needs `minimum` 2 insns unless the scanned tail is preceded by a
  CODE_LABEL (`--minimum`) or the mismatching insn is a jump around the scanned jump. In a leaf function
  the first `li r3,N; blr` copy therefore survives and later copies jump back to it - unless a copy is
  followed by a block that starts with a set of r3 (`lhz r3` after `ble`): the jump.c "x = a; goto l"
  block is entered, fails, and its `if (changed) continue;` skips the cross-jump section for that
  copy in the first iteration (GetEtcAmbType: R404 deleted into R10C in iteration 2). In non-leaf
  functions all copies are `b end` and the latest copy always wins (isKamae). `-DSMALL_REGISTER_CLASSES=1`
  in a scratch cc1plus reproduces GetEtcAmbType's return-copy survival (jump.c refuses to hoist
  sets of hard registers) but breaks its store-flag tail and does not fix isKamae, so it is not the
  original's difference either.
- `if (a == 6 || a == 3) return 0; return 1;` keeps `beq ret0` twice (the label between the compare and
  `li r3,0` blocks the "x = a; goto l" hoist); two separate ifs give `li r3,0; beq end` for the last.
- A shared `return 0` reached by `goto ng` from an earlier arm gives `beq ng; li r3,1; b end` where a
  plain `return 0` followed by `return 1` is hoisted into `li 1; bne end; li 0` (pl_sub joyKamae).
- `path = pUser_name;` reassigning a parameter that is later dead: the pseudo gets a second set, is
  global-allocated and a phantom callee-saved register (`stmw r30` with r31 never referenced) is
  saved; reading the global directly in each arm gives `stw r31` (file_app file_lock).
- `cPlayer* pl = pPL;` before a `switch` hoists the `lwz pPL` above the compare tree (pl_sub
  PlReloadBullet); `const f32 limit = C;` keeps the pool `lis` in a callee-saved GPR across a call and
  issues the `lfs` after it (a plain `f32` local loads f31 before the call, the literal loads both
  after it) (SubCharCheckHealing).
- List walks that test `next` at the bottom (`mr r4,cur; lwz next; ...; cmpwi next; bne`) with an entry
  test on the head are `if (p) { do { cur = p; next = cur->next; p = next; ... } while (next); }`;
  `while (p)` with a switch body is not rotated (PlDataRelease).
- COMPILER-DIFF 4 view idiom for a u16 accumulator: `u16 total = numS(id)` with
  `u16 numS(int) asm("num__8cItemMgri")` drops the `clrlwi 16` on the int result (item bulletNumTotal).
- Remaining register-allocation residuals, all ~1 insn: objWep drawPoint (a block-local `esp` reload
  gets r11 in the target, r9 in ours; priority order of the local-alloc qtys), lightPath movePath
  (`this` 8 refs/34 insns = 7058 vs `pCur` 5/14 = 7142: pCur wins r9; `this` needs 9 refs or a 33-insn
  range), dvd ErrCheck (pMes/pStr 390/386 -> 76/77 buckets; the *first-set* pointer has the longer
  range by 4 whatever the declaration order), rnd Rnd (the `m = n` copy survives in the target because
  cse did not rewrite `m = n + 0x101` through `m`), item trigger (`clrlwi r4,r9,16` on a u16 member
  shared between a compare and a u16 call argument: COMPILER-DIFF 2 class, five compare forms tried),
  item init (`li r3,0x20` before `addi r4,__FILE__`), at_mod ComnHitCheck (first MTX_COPY counter in
  callee-saved r30 and the second copy's `dp_/sp_` both copied: the five s_/d_/i_ declaration orders
  do not change it).

- Object/misc units (obj02, obj03, obj12, objPillar matching; obj00 7/8, obj1b 13/15, objRobo 15/19,
  pad 11/12, room_jmp 14/17, filter06 5/7):
  - Judging tool used: masked-word compare of every function (union of both objects' reloc fields
    masked, same-section `bc`/`bl` relocs resolved to displacements, reloc targets compared as
    `section+offset` with weak vtable symbols accepted by name or own copy) plus `.rodata/.data/.sdata`
    word compares. objdiff's "99%" REPLACE rows on obj03/obj00 were all dtk `lbl+off` spellings.
  - A dead-stripped out-of-line member whose pool survives is defined *between* the live functions
    whose pools bracket it: obj03 `int cObj03::init()` between the ctor and `move` (one SF 0.0 store
    puts the 4-byte zero before move's pool, which then needs the 8-byte pad for its DF magic), obj02
    `cObjScr::SetSwingRot(f32, f32, f32)` after moveSwingRot with `1.0f / period`, `*= 10000.0f`,
    `* 6.2831855f` (pool [1.0, 10000, 2pi]); both units in STRIP_UNUSED. A pool that starts 8-aligned
    with a 4-byte SF entry first (`0 | 40.0 | 0 | DF magic | 500`) is an SF pool of a dead function
    immediately before it, not an alignment rule.
  - `.sdata` 8 bytes for one `u16` static (obj03 `hist`) and `.rodata` 4 bytes longer than the last
    string (pad): `asm(".section .sdata; .balign 8")` / `asm(".section .rodata; .balign 8")` at the
    end of the unit (the split objects carry the 8-byte alignment).
  - The rope code is shared by obj00 FallMove, obj12 fallMove and obj1b_R1_Fall: function-scope
    `Obj*Node* p, *n` (final giv value `addi r0, node, 0x58` after the k loop), the dead
    `if (i == 2) n = node; else n = &node[i + 1];` block in the *hit* loop (obj12/obj1b have a switch
    with six fRand calls there: 114 RTL insns > loop.c's `threshold` 73, so without the compare the
    `&node[2]` bound is rematerialised at the loop bottom instead of `mr r26, r0` in the preheader),
    one `f32 mag` for both `PSVECMag` in the k loop and the final speed sum (two assignments ->
    `fmr f12, f1` after the call), `diff = (p->len - mag) * 0.5f; PSVECScale(&d, &d, (1.0f / mag) *
    diff)`, and `obj->pos = d` written *before* the speed sum (the copy's word temps r9/r11 follow).
  - obj1b setFall: the switch on `i` has `default:` sharing `case 0:` (the dispatch falls into case 0,
    no `b end`); with that CFG haifa's speculative interblock motion pulls case 2's unreduced address
    insns (`mulli i*12`, three `addi w+0x20/24/28`) into the dispatch block and copies them into case 1's
    tail (update blocks). The angle test is `if (dir->x == 0.0f && dir->z == 0.0f) ang = 0.0f; else ang
    = atan2f(dir->x, dir->z);` (the `&&` form: the else label has two jump uses, so cse cannot follow
    into it and case 1 reloads both operands while case 2 gets the PRE copy `fmr f13, f0`); the `||`
    form matches neither case.
  - A pool `lis` kept in a callee-saved register *across a call* for one later load (objPillar
    plemEscape `lis r30, 0.0@ha` before `bl Muku`): a dead `ang = 0.0f;` as the first statement of that
    case block. The high pseudo is then set and used in the same basic block (REG_BASIC_BLOCK >= 0), so
    update_equiv_regs does not move the `lis` next to its use; a function-top `f32 ang = 0.0f` puts the
    set in block 0 and the `lis` lands after the call (`lis r9`).
  - `obj->pos = obj->getPartsPtr(0)->worldPos` reuses r3 for the source address (`addi r3, r3, 0x70`);
    `parts = obj->getPartsPtr(0); obj->pos = parts->worldPos;` keeps the call result (`addi r11, r3,
    0x70`). `v = pPL->pos` loads pPL before the following `lfs 1000.0`; the struct view `pPLS->pos`
    (cam_ctrl.cpp's `PlayerPtr`) issues the constant first (objPillar R0_Throw).
  - VibSetDataCore (pad): store order `type, time, wait, level, add` (level last-but-one) gives the
    target's `stw lvl` after the `lhz wait`; the other 119 permutations were tried by script.
  - room_jmp getRoomInfo: `u32 base` (not `u8*`) with `u32 o = idx * 32 + 4; return (CRoomInfo*)(base
    + o)` gives the target's `slwi; addi 4; add` tail (a pointer local folds the +4 into the base); the
    remaining r0/r11/r9 assignment of ofs/n/base is open.
  - OPEN, register only: obj00 FallMove's third-loop QI `1` gets r24 (ours) vs r23 (target): global.c
    allocates A = the k loop's `k+1` giv (refs 4/len 46, prio 1739) and C = the k loop's SI `1` (7/102,
    1372) before B = the hit loop's QI `1` (3/82 after the REG_EQUIV doubling, 365); B then takes the
    first free register r24 in pass 0 because A already used it. The target's B took r23, i.e. r24 was
    not yet "used so far" (A allocated after B) or conflicting; do-while/int-k/u8/shared-`one` forms do
    not change the three priorities.
  - OPEN (obj1b SetSpear, dying-store family): a 30-store work init where the 0xFF and 0 pseudos are
    shared; ours issues the last use of each (`stb 0x6c`, `stb 0xfe`) first, the target keeps pure
    source order (only the single-use 0x32 store is pulled forward). Moving the last statements does not
    help (they stay last); `int/u8 ff/zero` locals and an int-parameter routine inline change the code.
  - OPEN (obj1b obj1bHitCk): `mr r26, r28` (a second `&obj->pos` pseudo copied after `getPartsPtr`) used
    for VECNormalize/PSVECAdd while PSMTXMultVec keeps the first pseudo; 90 placements of a `Vec* p`
    local / `&obj->pos` uses tried (cse-skip-blocks folds any `p = &obj->pos` after the `if (partsNo)`
    block into the argument pseudo).
  - OPEN (objRobo R0Init): `lwz pG` for the SetEmHit arguments is issued after the three `li`s in the
    target (ours between `li r5` and `li r6`), a store->load latency effect; `PSet(w->hit[i], 0)` breaks
    the giv. TaskSwitchFront/Back: `max`/`range` FPR swap (f29/f30); ours has REG_LIVE_LENGTH 136 vs 80
    for the two hoisted loop invariants, declaration/statement order and inline forms do not move it.
  - pl_wep/pl_sub: `EspDataRelease` is now declared in esp.h (`extern "C" int (u32, int, int)`); a
    unit-local C declaration with `int` parameters is a compile error.

- Tools REL debug-tool units (src/Tools/, 2026-09): the module (and t_event, the same db_toolbase object)
  is compiled with `-fno-implement-inlines` (modules.py CFLAGS): `cDbgWindow::AddButton` inlines the
  in-class `cDbgButton(x, y, name, cx, cy)` constructor (`pButton[num] = b = new cDbgButton(...)`: the
  slot address is computed before `__builtin_new` because the RHS is not a CALL_EXPR; a helper call as RHS
  evaluates the call first) and no out-of-line body exists although cDbgButton's vtable is in the object.
  The ctor calls the header inline `cDbgButtonBase::Init` (its message string opens the .rodata).
  db_toolbase.h's eleven display strings sit in a non-polymorphic struct of string-returning inlines.
- Two adjacent unit boundaries were wrong: a `cFlag.set()` string right after a unit's vtables / pools
  belongs to the NEXT unit's atari.h group (Tools t_atari 0x2578, t_rck 0x5000), like the rooms.
- `14.0f + 2.0f` unfolded in the target (LocalDisp) = both operands are variables: `f32 fh = 14.0f;
  f32 mgn = 2.0f;` declared in the same block are NOT folded by cse (the pool loads stay), while two
  literals fold at the tree level; the pool order is declaration order of the variables, so put the
  `(f32)` conversions (2^52 magic first) before them.
- `(y + 1 + b->y)` reassociates to `addi (b->y),1; add`; the target's `addi y,1; add b->y` needs
  `int by = y + 1;` as a loop-body local.
- `(u8)(a*255) << 24` + ... with `+`: accumulate into one `u32 col` variable (`col += ...`) so combine
  never sees the non-overlapping operands and keeps `add` instead of `or`.
- A lone unreferenced 4-byte float word between two functions' pools (t_prim 10.0 before TprimDrawHtr,
  t_util 0.0 before ToolMenuDisp) is reproduced by a public `const f32` object defined at that point
  (`extern const f32 X; const f32 X = C;`); unused locals, dead expressions and `static const` locals
  emit nothing (mark_constant_pool drops unreferenced pool entries).
- A shared source can carry the "full" build of a file behind a define (`TPRIM_FULL`, `T_UTIL_FULL`;
  src/Tools/t_prim.cpp and t_util.cpp are 3-line includes): the extra functions' pools replace the
  parse-time template hacks the dead-stripped builds needed (their constants were those pools).
  `configure.py`/tools/project.py now prefers an exact-case directory match, so src/Tools and
  src/tools coexist.
- Byte-store chains `p->a = p->b = 0` load the pointer once for both stores; a following separate byte
  store reloads it (t_mv `pMv->step = pMv->cursor = 0; pMv->camMode = 0;`).
- A `u8 zero = 0` (or `int ret = 0`) function-scope local kept in a callee-saved register reappears as
  the source of a byte store in a branch arm while the other arm has its own `li rX,0`; ours merges
  the literal arm into the variable (cse src_related through the SImode pseudo) and cross-jumps the
  arms — OPEN (t_mv mvInit). Also OPEN: cDbgWindow::Init's zero stores are issued in RTL order with
  no dying-store hoisting although the zero and `this` die at the last one (every reference-setter,
  chain, volatile and return-value form tried).
- Vec copies the target does with `lfs/stfs` are memberwise assignments; `*out = cur` gives `lwz/stw`.
- `for (i = 0; i < 4; i++) f(&p[i])` with `int i`: the strength-reduced pointer loop compares with
  `cmpw` (signed); the switch on the search result needs an explicit `case 0: break;` for the
  `cmpwi 1; beq; ble default` tree shape.
- Tools/tools.cpp: the split object is src/tools/tools.cpp (4 functions + the cLight block match) PLUS the
  module's .gnu.linkonce orphans (ToolArrayPush/ToolWorkPop inline bodies, cManager<T>::arrayPush/Pop of
  six managers) that the original ELF appended after .text; no compiled unit emits them yet, so the
  unit stays unmatched and callers only declare `void ToolArrayPush(int)` / `ToolWorkPop(int)`.
  t_mv also needs `SetToolLight(int)` global (db_light_tools.cpp has it `static`; the .sym scope is wrong).

- An uninitialised int argument (no r4 setup) is reproduced only by an asm-labelled free declaration
  `void f(cEm*) asm("setCloseLock__7cEmDoori")`; `int lock;` in any scope gives `mr r4, rX`.
- Pool order for a float-heavy call: `const f32 w, h, x, z` in a block right before the call (pool
  order = declaration order, no `lis` kept in a GPR); a function-top `const f32` keeps the `lis`.
- Locals <= 8 bytes get frame slots after every larger aggregate, in first-`&` order; aggregates
  >= 12 bytes at declaration in expansion order.
- `int cut = K;` set before a call and stored after it puts K in a callee-saved register.
- `case 0: break;` keeps the `cmpwi/beq end` node; `case 0: break; case 2: break;` gives the root-1
  tree with `ble end`; `case 0x13: case 0x14:` gives the range test, `a <= hi && a >= lo` folds.
- `.rodata` 8-alignment with no double constant: `asm(".section .rodata\n\t.balign 8\n\t.text")` as
  the first line of the unit (before header strings).
- COMPILER-DIFF candidate #9: loop exit-test duplication -- original `sub; b test; L: sleep; sub;
  test: bge L` (no duplicated entry test); our jump1 always duplicates it on rotated loops
  (r103/r105 `execOpenCover`).
- Tools REL, second pass (t_mes, t_cons, tools.cpp Matching; t_tplview 3/4, 2026-09):
  - Tools' `tools.cpp` needs `-DTOOLS_ARRAY` (modules.py CFLAGS, like t_id): with it the object is
    byte-identical including the six `cManager<T>::arrayPush/arrayPop` instantiations behind the cLight
    block (the module linkonce rule places them; no ngccc.py change needed).
  - `SetToolLight` is `static` inside db_light.cpp behind DB_LIGHT_SET_TOOL_LIGHT; the Tools wrapper
    (tools/db_light_tools.cpp) exports it with `asm(".globl SetToolLight__Fi")` — the object is otherwise
    unchanged and sync_rel_symbols flips the symbols.txt scope to global.
  - Menu list loops (`eprintf(x, y + i*14, 0, 0, tbl[i])`, 2-4 entries, reversed `subic./bne` with the
    y and pointer givs initialised AFTER the PRE'd `lis` of a later string): an inlined `static inline
    dispList(x, y, tbl, n)` helper. In place, the source-level inits precede the gcse insertion and the
    biv is eliminated for a pointer compare (`cmpw; ble`). With a *frame-address* table whose `&tbl`
    pseudo is gcse's reaching reg (`addi r11,r1,N; mr r24,r11` at the top), cprop puts that
    pointer-flagged reg into the giv and maybe_eliminate_biv wins over the reversal: only the `*tbl++`
    biv form reverses there (t_tplview main menu), the `tbl[i]` giv form is needed for a fresh table
    (t_tplview size menu) — OPEN: one helper cannot give both.
  - A string literal inside a `static inline` is emitted when the helper is parsed; when the target
    puts it after a function's aggregate templates, the templates are public `const` objects defined
    between (`extern const TplMenu3 x; const TplMenu3 x = {{..}};`, copied with `TplMenu3 m = x;`).
  - `wk->a = wk->b = 0` chain then w/h/step stores: the chain's RTL order is a, b (both from one zero
    pseudo), so b's store dies and is issued first, a's last (t_tplview reload block: `y, w, h, step, x`
    comes from `wk->y = wk->x = 0; w; h; step`).
  - Byte extraction of a colour word: `r = c >> 24; g = (c >> 16) & 0xFF; b = (c >> 8) & 0xFF; a = c &
    0xFF` — the masks survive as `extrwi`, the `& 0xFF` of the memory operand becomes `lbz 3(rTbl)`;
    four address-taken `u8` locals (frame 0x20..0x23, direct `stb N(r1)`), not a `u8[4]`/GXColor (SImode
    aggregate -> ADDRESSOF pseudo, `addi rX,r1,N; mr` COMPILER-DIFF #3 shape).
  - Fold reassociation (`fold-const.c associate`): `A + (B + C)` becomes `(A + C) + B`; to keep the
    target's `srwi; addi; add(shift, n)` write the operands into locals in the target's evaluation
    order (`u32 s = num >> 5; u32 n = num + 2; (s + n) * 4`); `ofs + 8 + (u32) buf` needs
    `u32 ofs2 = ofs + 8;` first (t_cons save_main/read_main).
  - `memcpy` with a `(void*)`-cast operand is a plain prototyped call; the target's `crclr cr1eq` block
    move needs typed pointers on both operands (`(u32*)`).
  - `t->num = i = 0` (chain) makes the loop counter's `li r9,0` the stored zero; separate statements
    give two zero registers.
  - `if (flag) ret = 0; else { if (c != lim) cursor = lim; else ret = 0; }` keeps the outer `li r27,0;
    b end` and jumps the inner arm into it; with the inner arms swapped ours cross-jumps into the last
    copy (COMPILER-DIFF #6 shape avoided by arm order).
  - Deleting a plain buffer: `delete buf` (`__builtin_delete`, no null test); `delete[]` gives
    `cmpwi/beq` + `__builtin_vec_delete`.
  - OPEN (t_mv mvInit, unchanged): `pMv->cursor = 0` in the then arm is cse'd to the `u8 zero` register
    and cross-jumped with the else arm's `cursor = zero`; the target keeps `li r10,0` in the then arm.

### Small tool RELs (t_light/t_light, t_id/tools+db_path, t_movie/t_movie+t_prim, t_event+t_sce/db_filelist matching; db_sctrl 15/22)

- The "linkonce orphan" tail of t_id/t_esp/Tools (ToolArrayPush/ToolWorkPop[/ToolEmArraySet], one
  cManager<cLight> block, cManager<T>::arrayPush/arrayPop x6) is tools.cpp itself built with
  `-DTOOLS_ARRAY` (modules.py CFLAGS; t_esp also `-DTOOLS_EM_ARRAY`): plain functions after
  `_unresolved`, then the deferred template bodies in instantiation order. `include/tools.h` declares
  them; `cManager<T>::arrayPush(int)/arrayPop()` live in cManager.h; the Esp/Espgen pool swaps are
  `extern "C" int` in esp.h/espgen.h, `ConsGetRoomValue` in cons.h. The bits of the flag word
  *exclude* a pool (`if (!(flags & 1))` — bit 0 is the `xori/andi.` form).
- `if (busy) return 0; body; return 1;` keeps `li r3,0` out of line after the body (`b end; li r3,0`);
  `if (free) { body; return 1; } return 0;` gets the `li r3,0` hoisted above the branch (4 bytes short).
- `t_id/linkonce.cpp`-style synthetic units are never right: a tail of named linkonce copies after the
  last object is the last object's own deferred output. Likewise a "unit" whose ctor key is not its
  first global function is two objects (t_event's db_filelist = Tools' db_toolbase.o + db_filelist.o).
- Unreferenced .bss of a unit whose own code never touches it (the global `cFileList DbgFileList`,
  driven by t_event.cpp) is attributed to the previous unit by the generator: pin it (`{".bss": 0xAC}`)
  and name the label by hand in symbols.txt/sym_map.tsv (data labels are not synced).
- A stray `DF magic + 1.0f` pool at the START of the next unit's .rodata (t_id/t_event db_sctrl at
  0xF0/0x1640) is that unit's dead-stripped leading function (never-called `static f32 f(int i)
  { return (f32) i + 1.0f; }` + STRIP_UNUSED), not a tail of the previous unit.
- `cString("...")` temporaries share a freed aggregate slot only because the class has a user copy
  constructor (BLKmode): cString.h declares `cString(const cString&)` (never defined; the DOL has none);
  with an SImode class the temp gets its own slot (t_movie movie_test_main frame 0xC0 vs 0xC8).
- `int col = (c) ? 6 : 0; f(..., (u8) col, ...)` gives the target's `clrlwi r7,r7,24` at the call; a
  `u8 col` local (either form) folds it away. A u8 colour passed as int in several arms with one
  PRE'd `clrlwi rX, rCol, 24` (db_sctrl sctrlMenu) is an `int col` set in two places (a dead `int col
  = 0;` before the block plus `col = 0;` in the loop: REG_N_SETS 2 stops gcse cprop) passed as `(u8) col`.
- Passing a struct by value to a varargs `%s` copies it to a temp and passes the address: movie_test_main's
  0x24-byte `MovieFile f = movie_file[i]` copy is the plain local form (block move loop of 0x18 + 0xC).
- `while (*p) { int c = *p; if (islower(c)) c -= 0x20; *p = c; p++; }` with `int c` keeps `subi` in
  place (`char c` re-extends with `extsb`); `islower` is newlib's `(_ctype_ + 1)[(int)(c)] & _L`.
- `pGS->debug_mode = ...` / `(*(u32*)((u8*)pGS + 0x68) & bit)` (the struct-view pG) keep the `lwz pG`
  below a preceding store through the work pointer where plain `pG`/`TOOL_FLAG` let ours hoist it.
- Clamps on s8 members: `w->cursor = w->cursor < 0 ? 0 : (w->cursor > 1 ? 1 : w->cursor);` gives the
  target's `extsb r9,r0 ... li r0,0/1 ... stb r0` with the raw byte kept for the store; `s8 c = ...` locals
  or if/else chains extend in place.
- `FuncPathWork`-sized locals accessed through a pointer (`PathParam buf; FuncPathWork* param =
  (FuncPathWork*) &buf;`) keep `&buf` in a callee-saved register (`lwz r0,4(r29)`); direct member reads
  address the frame (db_path pathGrabLine/pathDraw).
- `if (i > 0)` inside a loop body full of calls is the `cmpwi; mfcr rI; ... mtcrf 128, rI` CR spill
  when `i`'s register is dead after `i+1` was computed (pathDraw, ours reproduces it).
- A struct copy plus a byte store: `w->insertIdx = t + 1; w->insertPos = pt;` (byte store first in
  source) gives `stw x; stb; stw y; stw z` (the stb waits for its `psq_st/lbz` latency chain).
- Two consecutive stores of one value into two Vec locals: `g1.x = g0.x = v;` stores g1 first (chain
  order), `g0.x = g1.x = v` the other way.
- `int sx = (int)(v * 256.0f / 320.0f + 256.0f)` / `(int)(224.0f - y * 224.0f / 240.0f)` are the
  screen-position conversions of the tool cursors (drawCursor); drawAxis' label y is `224.0f - w1.y`.
- Rooms of unresolved diffs in db_sctrl: DbSctrl's `w->x/w->y/w->blink++` store order (target y, x, blink;
  no statement order gives it), SctrlAdjustAxisRange's two stepping pointers, drawAxis' early `lis`
  of the format string, drawScurve's `i`/`&wp` register swap (r26/r27), sctrlMenu's cross-jump of the
  case-1 cursor call.

### Single-unit enemy modules (em2e, em26, em18, em34, em24 Matching; em30 22/23, em3a 36/39, em3c 43/47, em38 71/76, em22 64/72; src/<em>/<em>.cpp, 2026-09)

- Layout of every one-file enemy REL: `_prolog` (`OSReport("<em> prolog Ok\n")` + `EmInitFunc = EmXXInit`;
  em2e has no OSReport), empty `_epilog`/`_unresolved`, `EmXXInit` = `new (em) cEmXX()`, `emXXDmCk`, the
  class `move` (damage check, `EmXX_R0_move_tbl[xFC](this)`, `EmMgr.destroy` on 0xFF, then
  `EmAtCheck/atari.move()/SatMgr.check(this, 0)`), the static routines, the free helpers, and the
  cUnit linkonce copies at the end. `.data` = the routine tables in definition order (the R0 table is
  a non-static global, the others `static`), then any static tables (em26 `u16 flip[32]`, `EmAtkInfo`).
  `.rodata` = [cFlag.set()][atari.h][light.h][dmg.h][ctrl.h...] header strings in include order (they
  prove the include set), the prolog string, then per-function strings/`static const Vec`s/pools.
  The 0x34-byte COMMON block of a module with `common_size: 52` (em18) is `asm(".comm common_em18,52,4")`.
- tools/ngccc.py `place_linkonce_module`: unnamed `.gnu.linkonce.t.*` copies of the module's FIRST
  object are dropped (an unreferenced first copy vanished in the original link) — the single-unit
  modules include light.h and would otherwise carry the 0x3B8 cManager<cLight> block.
- Types: `Em2eWork`/`Em30Work`/... overlay cEm at 0x3E0 (include/em2e.h ...); `hit[N]` EmHitInfo boxes
  at +0xC (YarareAdd), the RouteCk block at +0x214..+0x258 (em30/em34: routeAng, routeAngAbs, subAng,
  subAngAbs, targetAng, targetAngAbs, targetDist, routePos, subRoutePos, targetPos, pTarget, neckAng),
  `PlCloth cloth1/cloth2` at +0x25C/+0x2BC.
- Damage switches: reproduce the compare tree by listing the default-grouped cases explicitly (they
  are real nodes: em30 `5,6,D,E,F,12,13,2C,2D`; em26 `5,6,D,E,F,12,13,29,2C,2D`; em34 `5,6,D,E,F,12,13`;
  em18 BloodSet `case 0: case 0x14: default: break;`), and give a case its own duplicate arm when its
  node is not merged with an adjacent same-label node (em26 `case 0x2B:` written FIRST with a copy of
  the X0 body: `bne default; b X0` remnant; em18 BloodSet's [9..A] and [B..C,0x10] X0 arms). The
  survivor of identical arms is the last one in source order and falls into the shared `li r6..bl` tail;
  the `default:` arm is written last and keeps its own full call (block ends in the call).
- `switch ((u32) no) { case 0: default: ... case 1: ... case 2: ... }` gives `cmpwi 1; beq; cmplwi 1;
  blt default; cmpwi 2; beq` (em30SetParasite, em26 Dm_Small kind); the int-promoted `switch (em->type)`
  with `case 0: default:` gives `cmpwi 1; beq; ble default; cmpwi 3; ble/beq` (em34's type switches,
  default body laid out first when written first).
- `EmRoutineSet(em, fc, fd, fe, ff)` (int inline) is the default routine store; em30_R0_Init needs
  PLAIN byte stores instead: with the int inline the SI zero pseudo lands in r9 and reload_cse deletes
  the following MotionSetCore `li r9, 0`; a QImode zero (`em->xFD = 0`) keeps it (mode-size check).
- Shared constant pseudos across calls: em2e `int zero` for `lockParts = zero; w->flags = zero;
  EmRoutineSet(em, 1, zero, ..)` (the QI `lockParts = 0` before any SI zero would get its own `li`);
  `int one = 1` declared mid-block after the call the target's `li rN, 1` follows (em18 before
  `setStatus(one)`, em34 after YarareInit) for the routine `1` kept in a callee-saved register.
- Argument load order: `void* tplE = ARC(0xF);` before `ModInfoMgr.create(ARC(0xE), tplE)` puts the
  0xF offset load first (em18); `void* binR = ARC(0x13)` at the top precomputes the second create's
  bin across the first call (em18HandSet); a `void* tpl = ARC(8)` local after the modelInit check.
- `if (em->modelInit(ARC(4), ARC(5)) == 0) { pLog->err(..); xFC = 0xFF; return; }` inside EACH type arm:
  identical strings let jump2 merge the arms from `add r4` on (em26, em34 em33 arms), different strings
  keep them apart (em2e). `ok = modelInit(...)` in the arms + one check after does not merge (call at
  block end).
- Player/partner loads after work stores: `w->pTarget = pPLS` / `pSUBS` (struct views, defined per
  unit) keep `lwz pPL` below the preceding `stfs` (em30/em34 RouteCk); `pGS->room_id32`, `pGS->flags_170`
  the same for pG (em26 R0_Init, em18 Trade); `BitOn(w->flags, 0x20)` before `EmRoutineSet` + `pPL->dmg.set`
  (em18 TradeAction).
- Distance tests are inline expressions: `(a.x - b.x) * (a.x - b.x) + (a.y - b.y) * ... < K` (em2e DmCk,
  em26 near check with `Camera* cam = &pG->Cam;` computed before the getPartsPtr call); f32 locals give
  another register/issue order.
- em2e: `w->timer = Rnd() % 90 + 60` (the `%` is shortened to u8: `clrlwi 24`), the known-zero reuse
  (`if (w->timer) w->timer--; else {RS(1,..,0)}` stores the zeros from the timer register), `Vec spd`
  memberwise `= 0, 0, 10` per routine, SetWallMatrix's `VECNormalize` needs `#line 694`.
- em18: `KeyStop(0xEFCF0000)` (u64 arg: `li r3, 0; lis r4`), `SndCall(8, n, &em->pos, em->id, 0, 0)`
  (last arg 0, not em); a hidden `case 2: default: break;` in R1_Die_Normal's xFE switch (`cmpwi 1;
  beq; bgt end; cmpwi 0; bne end`); `fabsfE` ("=&f" early-clobber fabs asm) reproduces the
  COMPILER-DIFF #5 copy of the shared `fabsf(lp.y) > 700` check in ActEvtSetTrade.
- em26: unreferenced constants between em26_R1_Atk's pool and em26AtkCk's (`-400, 0, 2000, -500, 400`)
  are a `static const f32 [5]` inside em26AtkCk; its body is `if (w->atkHit) return 0;` (the early
  return's label keeps `mr r3, r31` before getPartsPtr) then a block with `EmAtkInfo* atk =
  &em26_atk_info; cModel* p = ...; int hit = ...;` (the `&info` lis/addi lands before the call) and a
  final `return 0` shared by both failure paths.
- em34: `static EmAtkInfo em34_atk_tbl[1]` + `static int em34_atk_pad = 0` (the 4 zero bytes after it);
  `em34AtkCk(em, no, parts)` indexes the table `no << 4`.
- OPEN (em30DmCk tail): the original ends the `hp > 0` else arm with `lbz r3, dmWep; cmpwi r3, 0x21`
  and no branch (r3 = return value): a `return em->dmWep` on the taken path of `== 0x21` whose branch
  only jump2 removed. Our cse folds the returned value to `li r3, 0x21` (record_jump_equiv on the
  fall-through, cse-skip-blocks on the taken path) in every form tried (if/switch, taken-path else
  arm, dead sibling arm, nested if, duplicated condition), and flow2's tidy_fallthru + life_analysis
  delete a dead compare, so the shape is out of reach: em30 stays 22/23 (+8 bytes), not Matching.
- em24 (Matching, include/em24.h; 2026-09): `switch (DmgMgr.hitCheck(&em->pos, 0)) { case 1: case 4:
  case 5: case 7: goto die; }` with the `die:` label inside the following `if (em->dmHit)` body (the
  shared `hp = 0; dmType = 0x80; EmRoutineSet(3,0,0,0)` tail); the weapon filter is an `||` chain
  (`cmpwi 0x14/0x16/0x17/0x2a/0xe` in source order, not a tree) on an `int wep = em->dmWep` read
  before the `dmHit = 0` store. `scale = (f32) (int) Rnd() * 0.15f * (1.0f / 256.0f) + 1.1f` gives the
  signed double trick (`(f32) Rnd()` alone is a `psq_l` fast cast); `int zero = 0; int two = 2;`
  after the scale stores feed `lockParts = zero`, `w->flags/stuckCnt = zero`, `splashTimer = two` and
  the default arm's `EmRoutineSet(em, 1, two, zero, zero)` while `case 0:` (written after `default:`)
  uses the plain constants (cse takes the `x38D` register). Six independent stores come out in the
  target order only as `flags; stuckCnt; splashTimer; slopeRot.x; .y; .z`. `switch (xFD)` with a
  `case 4: break;` after case 3 makes the 5-node tree (`cmpwi 2; beq; bgt; ...; cmpwi 3; beq; b end`).
  `fa -= fb` reuses the first floor's FPR for the difference; `f32 len = SQRTF(..); atan2f(fa, len)`
  loads the y operand after the call. R1_BoxWait's step 3 declares `cAtariInfo* at = &em->atari` in the
  `!(seFlags28B & 0x80)` block (two `|= 0x100` through one pointer) and ends the landing branch with
  `EmRoutineSet(em, 1, two, 0, 0); break;` followed by one shared `if (MotionMoveF(em, 0)) w->motEnd = 1`.
  R0_Die's fade: `if (w->timer) { alpha -= 0.05f; if (alpha < 0) {..; break;} } MotionMoveF(em, 0);`.
- em3a (36/39 identical, not Matching; include/em3a.h, 2026-09): the helicopter. Idioms: the hover
  clamp `if (pos.y < fl + 1800) { f32 y = w->spd.y + 10; f32 max = 20; w->spd.y = y; if (y > max)
  w->spd.y = max; }` (the constant must be a pseudo created after the sum: `+=` then `if (spd.y > 20)`
  schedules the 20 load after the store and reuses f13); `FSet(em->rot.y, LIMIT_ANGLE(em->rot.y))`
  before `Muku(.., em->rot.y, PI)` keeps the result in f1 (`stfs f1` straight after the call - a plain
  member store gets `fmr f0, f1` and the pPL load hoisted above it); `pPLS->pos` for the Vec copies
  that follow a stack copy (em3aLookPLCk/FindPLCk/Chase, otherwise `lwz pPL` moves above the copy's
  stores); `int ret; if (hitEm == 0) {..; ret = 0;} else {..; ret = 1;} return ret;` for em3aGunHitCk
  (both `return`s directly lay the `hitEm != 0` arm first; the constant pool needs the `== 0` arm's
  30.0 before 22.0); `EmSetDieCntE(em) asm("EmSetDieCnt")` (the module passes the enemy); the
  `cmpwi 6; beq; ble default; cmpwi 7; beq` x38D switch is `case 5: default:` + cases 6/7; the
  `case 7` arm needs PLAIN byte stores before its MotionSetCore (em30 rule: the int inline's SI zero
  in r9 deletes the call's `li r9, 0`); `IntSet(w->timer, K)` (int& setter) for the difficulty
  table `timer = 46; if (pG->x4F88 <= 1) timer = 76; ...` (pG reloaded after every store); the
  patrol loops index `((u8*) pG->pRoomEmi)[i * 0x40 + 8]` with `u32 i` (cmplw) and re-read
  `w->pRoute` at every use in em3aPatrolUpdate (the `mr r11, r8` copies); `cEm3a::setAtkWait` is a
  virtual defined LAST in the .cpp (in-class it lands after `~cEm3a` among the linkonce copies).
  OPEN: em3a_R1_Fix issues `addi r31, em, 0x3e0` (w) before the getFloor arg moves and `lis SatMgr`
  before the 600.0 load (ours: w last); em3a_R1_B_HideWait's `ry = rot.y +/- 1.5deg` arms allocate
  the sum to f0 and the constant to f13 (ours swapped; ternary, if/else temp and operand order all
  give the same); em3aPatrolInit issues `lis pG@ha` after the `w->pRoute = 0` store (ours before;
  a reference store folds the address into `0x688(r3)` instead).
- em3c (43/47 byte-identical, not Matching; include/em3c.h, 2026-09): four-type humanoid with a
  parasite head object (em3cSetParasite = em30SetParasite's shape) and a head-burst cloth simulation
  (em3cPartsBomb*: five points per parts overlaid on cParts+0x128, `Em3cPartsBomb`). Idioms:
  - `if (em3cAtkCk(em, &v, no)) return 1;` followed by `p = GetPartsAddr(em->pParts, 0x1A);` gives
    `cmpwi r3; li r3, 1; bne END` (jump.c's `if (...) {x = a; goto l;} x = b;` simplification: the
    hard return register r3 of `return 1` matches the next call's r3 argument load, so the `li` moves
    above the branch). The call must be the FIRST statement after the `if` (before the Vec stores);
    a `return 0` followed by such a call gets the same `li r3, 0; bne END` shape.
  - Three-term squared distance `dx*dx + dy*dy + dz*dz`: combine folds the FIRST product into the
    fmadds, so the asm shows `fsubs/fmuls` on the *second* term first (`lfs 0x98` before `0x94`);
    read the source order from the fmadds operands, not from the first load. Two terms keep source
    order (`(pos.x - oldPos.x)^2 + (pos.z - oldPos.z)^2` in move: the target uses `oldPos - pos` for
    the first SQRTF and `pos - oldPos` for the second). A partner distance tested with the hp
    (`lha` scheduled inside the float chain, `fcmpu f13, f0`) is `if (pSUB) { f32 d = ...; if
    (pSUB->hp > 0 && d < K) ... }`; the `&&` form loads/compares hp first.
  - `cmpwi 3; bgt X; cmpwi 2; bge Y; X: li 0; b; Y: li 1` = `switch (em->type) { default: v = 0;
    break; case 2: case 3: v = 1; break; }` (a childless range node tests the high bound, then the
    low bound; `default:` written first lays its body first). `if (a > 3 || a < 2)` and every other
    if-form is range-folded to `subi 2; cmplwi 1`. Conversely `subi 1; cmplwi 1; bgt` (em3cFootSe)
    is `if (em->seNo == 1 || em->seNo == 2)`; a `case 1: case 2:` switch gives the two compares.
  - The dead `lbz bell_stat; cmpwi 0; beq L; cmpwi 1; L:` pair (also em10FindCk's TODO) is
    `f32 r; switch (pG->bell_stat) { case 0: r = 25000.0f; break; case 1: r = 25000.0f; break;
    default: r = 25000.0f; break; }` with `d < r * r` and `w->plDist < r`: the identical arms are
    cross-jumped, the compares survive, and `r` (three definitions) is never constant-folded so
    `fmuls r, r` is emitted. OPEN: the target issues `lfs r` and the `fmuls` late in the block
    (after the pos/bell loads); ours issues them first (all six source forms tried).
  - Damage switch lists (balance_case_nodes: root = node where the cumulative cost, 1 per single,
    2 per range, reaches `(n + ranges + 1) / 2`; exactly 3 nodes -> middle): the blood switch has
    `0..6, 9..C, E..11, 14, 15, 1B, 1D, 26..28, 2B, 2C` + default (arm A, written first), `D, 12, 13,
    29, 2D` (B), `7, 8, 21` (C), `17, 2A` (empty); the routine switch lists `0..4, 9..C, E, 10, 11,
    1B, 1D, 26..28, 2B` as `break`, `5, 6, D, F, 12, 13, 29, 2A, 2C, 2D` + default (arm written
    BEFORE the `7, 8, 21` arm so its body follows the tree and the `beq END` of `cmpwi 0xe` and
    `cmpwi 0x2b` merge). Two identical arms written as separate `case 2:` / `case 3:` bodies (not
    `case 2: case 3:`) reproduce two `beq` nodes to one label (em3cModelInit).
  - Player callbacks: `pl->subArc = PL_EM(pl)->subArc; pl->dmType = 2;` (this order gives the
    `stb` before the `stw`); `sub->rot.y = GetXZAngle(..); sub->rot.y += -0.17453292f; sub->rot.y =
    LIMIT_ANGLE(sub->rot.y);` gives the `fmr f0, f1` copy with the first store dropped;
    `w->actMode = 0; w->escaped = 1; pPLS->dmType = 2;` gets the 1/2 constants in r0 and the zero in
    r11. `FSet(pPL->rot.y, pPL->rot.y + Muku(..))` when pPL is reloaded after the store
    (em3cAtkCk); `pPLS->setNoSuspend(1)` after a `pG->flags_5010 |=` store; `pGS->x4F88` for the
    difficulty table stores through `w` (`w->timer = 20; if (pGS->x4F88 <= 2) w->timer = 10;`);
    `int far = 1; if (ang < PI / 12) far = 0;` with `ang` computed into an f32 local first so the
    `li 1` follows the Muku call.
  - `.data` whose size is not a multiple of 8 gets 4 pad bytes before the ngcld BSS tag:
    `asm(".section .data\n\t.balign 8\n\t.text")` after the last table (em34's `em34_atk_pad` is the
    same padding). `static f32 tbl[5][5][5] = { 0.0f };` (explicit zero) lands in `.data`.
  - `EmSetDieCntE(em) asm("EmSetDieCnt")` again; `GetPlPos(Vec*, cEm*, f32)` (em_sub.cpp, marked
    local in Bio4.sym) is called from the module and now declared in em_sub.h.
  - OPEN: em3c_R1_Die_Normal's case-0 then-arm materialises a fresh `li r0, 0` for the EstSet stack
    argument where ours reuses the switch register (xFE == 0, cse followed `beq case0`); every
    switch/if form tried keeps the reuse (-4 bytes). em3cPartsBombSet: `add r29, rBase, rMult` for
    `em3c_bomb_pt[kind]` and `add r0, rAdd, rTime` for `add + em3c_bomb_time[i]` (ours swapped; a
    pointer local gives the first, a u16 parameter the second but with a `clrlwi` at entry).
    em3cPartsBombControl (0x7C8 bytes, -12): structure identical, callee-saved assignment (em r18
    vs r17, no r21 vs r20, ...) and three spill-slot numbers differ.
- em38 (71/76 byte-identical, not Matching; include/em38.h, src/em38/em38.cpp, 2026-09): the lake boss,
  one class for five enemies (`type` 0 body, 1/2 tentacles, 3 upper, 4 lower body) that find each other
  through `EmMgr.pArray + EmMgr.size * i` (em38SearchParts); virtuals ckHeadUp / setAtkWait / setIn /
  ckIn / setCritical / ckCritical / ckDown; R1 table of 17 `{br_*, main}` pairs (`xFD * 2`,
  `xFD * 2 + 1`); work +0x5AC..+0x664 (pBody, pTent[2], pUpper, pLower, pInfo, two `Em38Para` roots,
  eye angles, obj00 weak point, blend motion `MotionWorkSub` copies at +0x668/+0x738 and a `Camera` at
  +0x808). `MotionWorkSub::speedRate` (+0xC0, em.h) was padding; a `cEm25` placeholder class supplies
  the parasite's virtual slot (`setBirth`); `GetWepDmVal` is declared locally (em10.h clashes).
  Residuals: em38BirthParasite (loop-invariant `li 0xf0` kept in the loop by the original), em38BloodSet
  (switch arm register allocation), em38AtkCk2 (r30/r31 em/no swap), plemEscape (0.0f/500.0f pool
  order), em38EscapeCamMove (pG caching / pool order).
- em22 (64/72 byte-identical incl. the linkonce copies, .data/.rodata identical, not Matching;
  include/em22.h, src/em22/em22.cpp, 2026-09): the dog. `setGoto(Vec*, int)` is virtual (cEm22 vtable
  0x60, so the module's `_vt.5cUnit` copy lands at 0x5D0). R1 table of 20 `{br_*, main}` pairs, R2 2,
  R3 1. Work: +0x14/+0x18 route angle/abs, +0x24..+0x30 target angle/abs/dist2/RouteCkPosToPosDis,
  +0x34 plRoutePos, +0x4C routePos, +0x5C blow speed, +0x68/+0x6C goto flag/pos, +0x78 `Camera`,
  +0x170 `EmHitInfo hit[5]`, +0x274 `cObj16* pPara[5]`, +0x288 `cObj16* pParaAtk[3]`, then the
  ctrl pointers / timers up to +0x2E0 (espKind). Idioms found:
  - Constant reuse across calls: after `em->dmHit = 0; em->dmType = 1;` the original's
    `EmRoutineSet(em, 2, 1, 0, 0)` in the `hp <= 0` / `flags & 8` arms reuses those 0/1 pseudos
    (`stb r30, 0xff; stb r28, 0xfd`, callee-saved across LifeDownSet2/SndCall). Only four direct
    `em->xFC = 2; em->xFD = 1; em->xFE = 0; em->xFF = 0;` stores reproduce it; the int-parameter
    `EmRoutineSet` inline keeps fresh `li`s. em22_R0_Init: `int zero;` assigned right after the
    modelInit check (`zero = 0; mot = MOTION(em);`) feeds `lockParts = zero` and the int zero stores.
  - Damage switch (em22DmCk): break arm `0..4, B, C, 10, 11, 1B, 1D, 26, 27, 2B`, then the default
    arm `5, 6, 9, A, D, E, F, 12, 13, 28, 29, 2A, 2C, 2D + default` written BEFORE `7, 8, 21`
    (jump.c deletes the earlier of two identical `RS; b end` tails, so the default body ends up after
    the near test). Blood switch (em22BloodSet): `7, 8, 21` first, then `1..4, B, C, 10, 11, 1B, 1D,
    26, 2B`, a separate `case 0x27:` arm with the same body (two `beq` nodes), `0, 14, 22..25, 2A +
    default: break;` (they shape the tree: node 0x14 makes 0x15's `blt END`), then the type-1 arm last.
    R0_Init's routine switch: `case 2:`, `case 4:`, `case 5:` as three arms each
    `EmRoutineSet(em, 1, em->x38D, 0, 0)` (one cross-jumped body, the `beq`s share it) and
    `case 1: EmRoutineSet(em, 1, 1, 0, 0)` (cse turns both 1s into the index register).
  - Motion-choice switches store `mot` / `blend` / `flip` locals and call MotionSetCore once after
    the switch (em22_R1_Dm_Small, the three Dm_Blow switches): the arms are just the ARC loads.
  - A helper written as an inline taking `w` (the run turn: `rot.y += Muku(..); rot.y = LIMIT_ANGLE
    (rot.y); em22DirMatrix(em, Muku(.., rot.y, PI))`) copies the LIMIT_ANGLE result (`fmr f0, f1`) before
    the store; a macro / open code keeps `stfs f1`. Struct-member views: `b = pPLS->pos` for the Vec
    copy after `a = em->pos` (Run, Threat's PlRunCk2 block, RouteCk), `RouteCkToPos(em, &pPLS->pos, ..)`,
    `w->pTarget = pPLS; RouteCkPosToPosDis(&em->pos, &pPLS->pos)` (pPL loaded twice), `SndCall(8, 0x21,
    &pPLS->pos, ..)`, `LifeDownSet2(pPLS, 10, 0, 0)`, `Muku(&em->pos, &pPLS->pos, ..)` after a `w->`
    store; `pGS->flags_60 & 0x4000` for the Draw_line3d check after `w->` stores.
  - Player callbacks: `pl->subArc = ((cEm22*) pPL->dmgType)->subArc` and `pl->xFE = ((cEm22*)
    pPL->dmgType)->xFE` read the enemy through pPL (mat / rot through `pl->dmgType`); `y =
    PL_EM(pl)->rot.y; pl->rot.z = 0.0f; pl->rot.y = y + PI; LIMIT_ANGLE(pl->rot.y);` (result dropped
    by the original); `(u32) PlGachaGet() < 15` for `cmplwi`.
  - One f32 local reused for every temporary of a function (em22NeckMove: Muku result, pitch,
    neckY/3, `fabsf(neckX)`, neckX/3) puts it in a callee-saved f30; em22_R1_Turn's `d = fabsf(ang)`
    reuses the case-1 `d`, em22_R1_Escape's `lim = fabsf(Muku(..))` the run-turn `lim` (f31 / f2).
    `len = SQRTF(..); f = -atan2f(d.y, len);` (the inner call into a local). `p->worldMat` (cCoord
    +0x3C) is the parts matrix the neck rotation is concatenated into.
  - `u16 step = (*(u16*) ARC(n) & 0x3FFF) / 5; ... MotSetObj16(obj, ARC(n), 4, step * i);` - the
    product is strength-reduced into the `add rF, rF, rStep` biv with its `li 0` in the preheader.
  - `no = em->seNo; if (no == 0) return; no--; switch (no)` puts the byte in r6 (the SE argument);
    `em->seNo = 0` sits inside each arm (the default arm skips it). `if (em->xFC != 3) return; if
    (em->xFD == 0)` - an `&&` of two adjacent u8 compares is merged into one `lwz; clrrwi; cmpw`.
    `if (st) { if (st <= 3) continue; }` - the `&&` form folds into `(st - 1) <= 2`. `if (pos.y > fl)
    break;` then fall through into the next case (R11B_B case 3 -> 4).
  - `if (w->gotoOn == 0) return 0; RS; return 1;` and `if (w->paraWait) { paraWait--; return 0; }
    return 1;` (the arm that falls through is the one written second).
  - Residuals (8): em22_R1_Wait (`li r10, 0` for the last RS hoisted above the `pG` load into the
    predecessor block; nested ifs, pGS, direct stores, a `hit` local all keep it in the RS block),
    em22_R1_Threat (first hitCheck block: pPL load / temp registers, identical to Run's matched
    blocks), em22_R1_Jump (`fmr f12, f1` copy of the getFloor result), em22DirMatrix (f0/f13 naming of
    the tilt blend), em22ParaSetMotWait/Atk (em/i r30/r29 swap), em22ParaSetMotAtkHit (arm order of
    the i == 1 / i == 2 bodies), em22FootSeControl (em/w r30/r31 swap).
- em35 (not started; 0xC9C0, 113 functions): R0 table 4, R1 35 `{br_*, main}` pairs (Wait, Walk,
  BigStep, Turn, Atk, LongAtk, AtkDouble, BearHug, Hook, Critical, CriticalHit, Atk2F, Catch, CatchHit,
  Divide, then the upper-body `U_*` set: Wait, Jump, JumpUp, JumpDown, DoubleJump, BackJump, Turn180,
  Step, BigStep, OverStep, StepUp, StepDown, HandAtk, Atk, Upper, AtkSpear, Divide, Crawl, CrawlTurn,
  JumpToBeam), R2 6 (Dm_Small, Spinal, Big, Frame, U_Fall, U_Crawl), R3 2 (Die_Normal, Die_Pose);
  virtuals setDiePose / setUpperStart; .data 0x868 (beam graph tables from +0x148, 36-byte entries of
  u8 links + two Vec, more tables at +0x3AC..+0x7B4), .rodata 0x8A0, .bss 0x34 (common); player
  callbacks plem35_BearHug / Sit / DmFall2F / DmHook / CriticalHit / DashEscape / DmStamp / CatchHit
  and two event cameras (em35EscapeCamMove, em35StampCamMove).
- Tools REL, third pass (t_atari 16/17 functions, t_dr 13/15; src/Tools/t_atari.cpp, t_dr.cpp, 2026-09):
  - `cSat` has a constructor, `cSat() : cUnit(1) { flags = 0; }` (include/atari.h): t_atari's two
    `static cSat tbl[10]` arrays are built by the static-init loop as `stw 1; stw _vt.4cSat; stb 0,0x2a`
    and ss_map's `cSat` locals show the same three stores. game/atari.cpp `cSatMgr::construct` hand-wrote
    the stores around `new (p) cSat()`; with the ctor it should be the placement new alone (not Matching,
    left to its owner). The unit also emits `_vt.4cSat` + a nameless `_vt.5cUnit` copy in .rodata and the
    cUnit inline dtor/beginEvent/endEvent/`__dl` copies after the static init (module linkonce rule).
  - .bss order with ctor'd file-scope arrays: [function-local statics, at their function] [ctor'd arrays,
    emitted while the static-init function is generated] [deferred plain file-scope statics]. t_atari's
    `oldPos`/`hitLine` are `static Vec` locals of plmove10/hitcheck, `atWork` a file-scope static.
  - A function reached through a routine table whose body also appears inline in another function of
    the unit, with the out-of-line copy placed right after that function (t_atari `wk_edit` inside
    `edit`): the source repeats the body; an `inline` copy would be deferred to the end of the file.
  - Empty `if (trg & A) {} else if (trg & B) {} else if ...` arms give the `andi.; bne end` chains
    (one `lwz` of the word, alternating r9/r11).
  - Index-first `lhzx rD, rIdx, rBase` with the index in BASE_REGS (r10) and `lfsx/add rD, rIdx, rBase`:
    `*(u16*) (w->cursor * 2 + (u32) &poly[no])` and `(Vec*) (idx * sizeof(Vec) + (u32) vtx)`.
  - `u8 r = Rnd() % 20; v.x += r;` gives `mulhwu 0xcccccccd; clrlwi 24; xoris 0x8000` (signed double
    trick on the zero-extended byte); `Vec v2; v2.x = v.x + n->x * 3000.0f` memberwise.
  - Zero-store pairs `mode = 0; plMode = 0;` come out `stw plMode; stw mode` (the later source store dies
    and is issued first) -- derive the source order from the target with the dying-first rule, every
    Tools routine has several of these.
  - t_dr has no entry in tools.cpp: `ToolDr`, `tDrInit` and a player-position display were unused
    `static inline` functions whose strings are still emitted in parse order ("[DATAREAD ...]" + the two
    path formats at the top, "[PLAYER]".."ANG:%f" between tDrArea_Move and tDr_getFilename,
    "CAMERA MODE" after tDrSaveDataCreate); the routine table is an unreferenced extern-linkage `const`
    array (`extern void (*const tDrFunc[5])();` then the definition) at the .rodata start.
  - The t_dr work pointer is a one-member struct (`DrWorkPtr drWork; DR = drWork.p`): every store
    through it, including `sth`/`stw`, reloads it (record alias set).
  - Local routine tables `void (*tbl[3])() = {a, b, c}; tbl[DR->step]();` are .rodata templates copied
    to the frame (3 lwz/stw) at the declaration point (between the previous function's and this
    function's strings).
  - s16 clamp with one store and no pointer reload: a `static inline` taking the work pointer with
    `u16 v = w->areaNo; if ((s16) v >= lo) { if ((s16) v > hi) v = hi; } else v = lo; w->areaNo = v;`
    gives `lhz; extsh; cmpwi; blt; cmpwi; ble; li hi; b; li lo; sth` (the u16 local keeps the HImode
    load; an s16 local or direct field compares give `lha`).
  - `n = top + 7; if (n > 128) end = 128; else end = n;` keeps `mr end, n; cmpwi n; ble; li end, 128`
    (the ternary and `end = n; if (n > 128)` forms coalesce the copy away).
  - A u8 colour that the target masks (`clrlwi`) at the call is `int col = c ? 6 : 0; f((u8) col)`
    (multi-set pseudo, nonzero_bits unknown); `u8 col = c ? 6 : 0` is never masked.
  - `p += strlen(p); p++;` (two statements) gives `add; addi 1`; `p += strlen(p) + 1` adds 1 first.
    `size = len + 0x10; saveSize = n * 0x38 + size;` keeps `addi len,0x10` separate from the `addi n*0x38,0x10`
    of the neighbouring store.
  - `DrArea* a = (DrArea*) (no * sizeof(DrArea) + (u32) DR->area); a->flag` (index-first add with the
    +0xA0 folded into the displacement) vs `DrArea* a = &DR->area[DR->areaNo]` (`mulli; addi 0xa0; add w, idx`)
    -- both shapes occur in the same unit.
  - Unit boundary: t_eminfo's .rodata pin is 0x3088 (its `cFlag.set()` string); the zero word at 0x3084 is
    the linker's pad to t_eminfo's 8-aligned .rodata and stays in t_dr's split object (ours ends at
    0x3084 -- do not `.balign 8` .rodata there: t_dr's .rodata starts 4-aligned at 0x2D6C).
  - OPEN t_atari plmove10 (19 words): `Vec old = w->pos;` (struct copy, lwz/stw) followed by the three
    `w->pos.x += ...` float RMW statements: the target issues the copy's `stw`s after the `stfs`s and the
    x/y `lfs` before the copy's `lwz`s; ours the reverse. Copy placement (top/after/pointer/memcpy),
    statement order, JOY pointer, `(f32)` factor order tried.
  - OPEN t_dr tDrArea_ListDisp (-8 bytes): the target keeps the function-top `lis drWork@ha` in caller-saved
    r6 for the straight-line blocks up to the `end` diamond and a second pseudo (`lis r31`) from the
    `listTop < areaNo - 6` block on, then reloads `drWork.p` for the `>` eprintf; ours PREs one r31
    pseudo for all and carries the pointer across the `end` diamond (cse-skip-blocks). tDrArea_Menu_main
    (`a`/`lis` r28<->r30) is global-alloc order only (five declaration orders tried).
  - t_motseq notes for the next pass (not written): the work is `Debug_alloc(0x1E10, 1)` behind a plain
    pointer at .data 0x21E8 (`static MsqWork* pMsq` -- reloaded after every store like the t_dr one), the
    routine table `.data 0x21EC` = {Model, SeqLoad, SeqMake, Sequence, SeqResize, File, QuitCk, Quit}
    indexed by `mode` (+0x10C0; +0x10C4/0x10C8/0x10CC sub states, +0x10D4 file sub), 0x220C is a
    21-entry `int` y table; the work holds `u16 num` (+0), `{u16 frame; u8 se; u8 flag}` frames from +4
    (1024 max, 0x40 = one motion frame, 0x280 = 10), a JOY copy at +0x10D8 (`on` 0x10E8, `trg` 0x10EC,
    `rep` 0x10F4), `u8 cursor` +0x10A2 (0..9 flag bits), +0x10A4..0x10B3 sixteen flag display bytes,
    `u32 viewFlag` +0x10BC (dbModSetViewFlag/dbModUnsetViewFlag), `u32 x15AC` (copy-clear colour word),
    `u8 se mode` +0x1DBC, `u32 seqNo` +0x1CA4 and the file name at +0x1CA8 ("%1d" digit patched before
    the '.'), start/end/add/max frames at +0x1DAC..0x1DB8, `dbModSlot[0].pModel` (+0x1F8 max frame, +0x290
    current frame, +0x294 motion count, +0x94 pos, +0xF4 parts list) everywhere; strings and pools in
    .rodata are in the order listed by secdump.

### Ganado shared library (em10/em10.cpp, 379/382 byte-identical, .data/.rodata/.bss identical, not Matching; 2026-09)

- The unit is shared by 16 modules (`common_<mod>` .comm block from `REL_MODULE`, everything else
  identical); flag MATCHING for all 16 at once, only when every function matches. em10.cpp is in
  STRIP_UNUSED: dead `static` placeholders (plem10NeckBreakCamMove, em10FindFloorCk) exist only to
  emit their orphaned constant pools. Idioms beyond the em24/em3a/em3c lists:
  - `Em10_R1_move_tbl` is a flat `Em10Func[220]` indexed `[xFD * 2]` / `[xFD * 2 + 1]` (`(idx << 3) | 4`
    in R0_Move); a `{br, main}` struct array gives `rlwinm; addi 4`.
  - `EmRoutineSet(em, 1, N, zero, zero)` with `int zero = 0;` when both zero args come from one
    register (em10DashCk, em10LadderResetCk); `int one = 1;` inside the `hp > 0` block for a
    hoisted `li 1` (br_Dash); PLAIN `xFF/xFC/xFD/xFE` byte stores when the target has them in that
    order (setGatling, em10DashCk's first set).
  - `pPLS->pos` for the Vec argument of `Muku`/`EM10_WEP_ATK_CK` (moves `lwz pPL` below the pos
    loads: R1_Threat, C_SawCriAtk, ThrowBomb, Scythe/Suki/ClawCri AtkCk); `pGS->` for a pG field
    read after a work store or in a loop (R1_Trade `flags_170 &= ~0x80000000`, SetTakeawayPosUpdate,
    SetWanderRoute `flags_60`, plem10_C_SawCriHit `pl_life`, plem10Kick `x4FB8`); `IntSet(w->x20, 1)`
    before the water-effect pG read (Dm_Roof).
  - `em->rot.y += Muku(..); em->rot.y = LIMIT_ANGLE(em->rot.y);` written in BOTH arms of an if/else
    (ShieldAtk, TorchFrame, Dm_Blow; cross-jumping merges the tail, a shared statement after the
    if/else gives a different label placement). Same for `em10SetDamageRack` in R1_WindowAtk.
  - `f32 a = fabsf(x); if (a < K)` (fabsf result in f1 with the compare's operand order:
    em10NeckMove, ActEvtSetTrade, ThrowBombCk/ThrowAxeCk's `ang`); `plDist2 < A || plDist2 > B`
    as two nested ifs (ThrowBomb/ThrowAxe Ck); `f32 len = SQRTF(..); -atan2f(d.y, len)` (ShotGatlingCk).
  - An early `return` form instead of a final if/else keeps a redundant `mr r3, r31` before a tail
    call (em10WanderRouteUpdate; the label invalidates reload_cse). `if (!em10FindCk2(em) ||
    (w->flags & 0x08000000)) return 0;` keeps `li r3, 0` below the `bne` (ClawStickCK; ChgParasiteCk
    additionally needs the final `if/else` returns flipped).
  - Narrow args: `em10CallVoiceSeI(cEm10*, int) asm("em10CallVoiceSe")` (COMPILER-DIFF, SetDamageVoice
    passes an int without `clrlwi`); `int t; Ctrl12SetS(.., (s16) t)` for an `extsh` (SetAtkWait);
    `int flag` + `(u16) flag` casts (Dm_Head/Dm_Flash).
  - Constant-pool order: a dead `const f32 k = 1.01f;` in the switch `default:` (R0_Init, setReset),
    `const f32 k = 30.0f;` (GatlingHitCk), `f32 r;` assigned in the first switch (FindCk); the
    `if/else if` chain on `pG->x4FB8` is a `switch` (ActEvtSetKick); `switch (e->x38D) { case 2: case
    3: case 4: }` for the `cmplwi` range (R100Cliff).
  - `EmiData` loops: `for (i = 0; i < emi->n; i++) { EmiEntry* e = &emi->entry[i]; ..continue; }`.
  - status: objdiff REPLACE/ARG_MISMATCH rows whose text is identical or whose reloc differs only by
    `R_PPC_NONE` are matches (`/tmp/em10w/status2.py` resolved reloc targets by name).
  - Third pass (348 -> 365, 2026-09-10; private harness /tmp/em10c: mcmp.py masked compare, sbs.sh
    side-by-side objdump diff, variants.py + pbuild.sh incl. strip_unused/fold_linkonce):
    - Long init store blocks (InitRtnSet): the target order is NOT the source order. Derive the
      source from the sched1 model: between two calls, stores whose source register crosses a call
      get a data dep to the next call (prio +2, issued first as a group), stores whose source was
      defined after the previous call get an anti dep (prio +1, issued after); inside a group, the
      store where a register dies (its LAST store in RTL) goes first, the rest in RTL order. So a
      target block `A B C D E` where A's source dies means the source wrote `B C D E A`
      (`x680, x598.x/y/z, x66C` after the f31 stores; `x524, x52C, x530, x528, x6AD`; Block B ends
      `pParasite = 0; x6B6 = 0; x58C = 0`; the post-loop block ends `x6C4 = 0; x594 = 0`).
    - `void** mot0 = &w->evtMot[0]` declared INSIDE the `for (i < 3)` body (both pointers): gcse PRE
      hoists the two `w + ofs` to the first block after the join (r28-based `addi`, callee-saved r27/
      r26) and the loop keeps `mr r8, r27; mr r10, r26` copies with one shared `i*4` giv (`stwx`).
    - `switch (em->x38D)` in InitRtnSet needs `case 0: default:` (tree root 0x11, left subtree root
      [7-9]); `switch (x38D) { case 0x14: case 0x15: break; default: ... }` gives the unfolded
      `cmpwi 21; bgt; cmpwi 20; bge` pair (an `if (x < 0x14 || x > 0x15)` is range-folded).
    - Six separate `pos.x = 0.0f; ... rot.z = 0.0f;` statements (chained `a = b = c = 0` reorders);
      `scaleBase.x/y/z = 1.0f` in x, y, z order (target `z, x, y` = last-store-first).
    - `a = fabsf(ang); if (a > K) continue;` with `f32 a` at FUNCTION scope set in three switch arms
      gives the untied `fabs f0, f1` (a multi-set pseudo is not local-allocated); the direct
      `if (fabsf(ang) > K)` ties `fabs f1, f1` (HideRtnCk/HideRtnCk2).
    - Routine stores inside loops (HideRtnCk arms): PLAIN `em->xFC = 1; xFD = 0x17; xFE = 0; xFF = 0`
      byte stores in all arms; the int inline's SI zero pseudos are combined and hoisted by loop.c
      (`li r22, 0` callee-saved), QI zeros stay `li r0, 0` inside each arm.
    - GotoPosCk: one `EmiEntry* f` reused for both inner loops (`mr r4, r11` giv copy, the register
      set r25..r31), one `u32 j` for all inner loops (r6), and a FUNCTION-scope `f32 d` for the
      inner-loop distances (`fmadds f0, f12, f12, f13`: the global pseudo takes f0, `dz` stays f12).
    - cEm10::move: `w->flags |= 0x10; w->flags |= 0x01000000;` (two statements -> `ori; oris` order;
      one `|= 0x01000010` gives `oris; ori`); the x38D switch written `default: break;` FIRST, then
      `case 5: case 6: case 0x1A:`, `case 7: case 8: case 9:`, `case 0xA: case 0xB: break;` (the
      jump.c "if (foo) bar; else break" swap needs the 5/6 body first and no `beq ARM; b END; ARM:`).
    - LostHead: `switch ((u32) a) { case 0: default: BODY; break; case 1: BODY (identical copy);
      break; case 2: ... }` -> `cmpwi 1; beq; cmplwi 1; blt; cmpwi 2; beq` (0 and 1 are separate
      nodes only when case 1 has its own body; cross-jumping merges the copies).
    - `int one = 1;` + `do { EmRoutineSet(em, one, 0x1B, 0, 0); } while (0);` keeps the `li r28, 1`
      at the declaration across the calls (R1_Wait, R320Gatling): update_equiv_regs moves a
      single-use constant next to its use only outside loop notes (`depth == 0`).
    - RouteCk: `BitOff(w->flags, 0x08000000)` / `BitOff(w->flags, 3)` where the target keeps
      `lwz pPL` below the flags store; `FSet(w->x524, RouteCkPosToPosDis(..))`, `FSet(w->x528, ..)`
      where the following pPL/pSUB loads stay below the store; `pGS->flags_60` after the x520 store.
    - setGotoSwitch: no `dst` pointer -- `w->x5F0 = *pos` / `= sw->pos`, `getFloor(&w->x5F0, ..)`,
      `w->x4EC = w->x5F0` (PRE copies `addi r9, w, 0x5F0; mr r30, r9` in each arm, x from `1520(w)`).
    - setWeaponFall: `u8 wtype = w->wepType` for the compares but `switch (w->wepType)` (direct
      member) for the QI pseudo + `clrlwi` re-extension shape; Pickup: plain `w->pWep && w->wepType
      == 4` / `== 9` member reads in both conditions (no temps).
    - R100TurnWalk: `arc = em->subArc; m0/m1 = PL_ARC_PTR(arc, ..)` first, the later if-arms read
      `em->subArc` directly (cse copies the load pseudo: `mr r10, r11`).
    - DmSetWep09: `no = part->partsNo; if (no == 5) no = 1; else no = 0x11;` (`lha r0; cmpwi r0,5;
      li r0,17; bne; li r0,1`: the else-set cannot pass the compare of the same register).
    - JumpUp: `w->x18 = f; w->x18 *= 0.010989011f;` (the multiply result lands in the constant's
      register f12, `x5A4.y` stored before `x18`).
    - DoorAtk: `if (em10SetDamageDoor(em, 1) != 1) { em10SetDamageRack(em, 2); break; }
      em10SetDamageRack(em, 0);` (the `== 1` form lays the arms out the other way round).
    - ThrowAxe: `pPLS->pos` in the `w->x4--` Muku call; Goto: `case 0xE: break;` after the [C-D]
      arm of the second x5EC switch (`ble ARM; b END`), and case 6's MotionSetCore with `m0/m1/flag`
      locals (the `addi r4, em, 0x1d8` after the flag branch).
  - Fourth pass (365 -> 374, 2026-09-10; harness copied to /tmp/em10d, `fdis.py SYM` prints one
    function with branch targets as function-relative offsets, `fn.sh DUMP FUNC` cuts one function
    out of a cc1plus dump):
    - cse follows at most 9 conditional jumps per extended block (cse.c PATHLENGTH 10): the 10th
      `if (c) return 0;` of a run is not followed, so the code after its `return` block starts a
      FRESH ebb — register equivalences (`w = em + 0x3E0`) and loaded values are forgotten there.
      CatchPLRtnCk's `w->flags` reload (`lwz r0, 0(r29)`, not merged with the `em->x3E0` word of
      the previous test) needs the four `em->type == K` tests written as SEPARATE ifs (an `||`
      chain is one jump). Conversely, `asm volatile("")`, `do {} while (0)` and `A || B` do not end
      a cse ebb (followed/AROUND paths keep the table); only labels hit by fallthrough and the
      path limit do.
    - A pool-constant `lfs` (`mem/u`) gets a sched1 TRUE dependence on every `mem/s` store that
      precedes it in RTL (this alias.c does not exempt unchanging loads), so a `= 0.0f` member
      store whose `lfs` the target hoists to the block top must be the FIRST statement of the block
      (em10HideOn: `alpha = 0.0f; be_flag &= ~2; dmType = 0x80; flags |= ..; atari.flags &= ..;
      x6B6 = 1;` gives the target's be_flag/atari/alpha/dmType/flags/x6B6 store order).
    - loop.c giv combination (`combine_givs`): equal-benefit address givs are combined into the one
      recorded LAST (giv_array is the reversed giv list), but a DEST_REG giv with a zero addend
      (`&emi->entry[i]` = `emi + i*64`, "ncav" +1 benefit) always wins. GetWanderRouteEmi's
      `addi r9, emi, 9` / `lbz -1(r9)` base (the `.sub` field) comes from a byte-offset entry
      address, `(EmiEntry*) ((u8*) emi + 8 + i * 0x40)`, whose DEST_REG giv has a constant addend;
      the second loop re-reads `pG->pRoomEmi` for the bound (gcse PRE copy `mr r8, r9`).
    - `int one = 1;`-style `do { EmRoutineSet(..); } while (0)` after a tested call: the loop notes
      keep `li r3, 1` (the `return 1`) below the routine stores, so the call result (known 0 on the
      fall-through, reused for the xFE/xFF zeros) stays in r3: `mr. r3, r3` + `stb r3` (RackBreakCk).
    - A struct-copy `pModel = m` store the target issues before a pool 0.0f store that is NOT the
      constant's last use: put the pointer store between the two 0.0f statements (Em1fClothSet
      `x44; pModel; x48 = 0.0f; x4C; x50 = 0.0f`) — weight-0 stores go in LUID order.
    - Crash: `int r = Rnd() % 5;` before the MotionSetCore statement (the `&em->mot` addi is then
      computed after the call and regmove ties it to r4, issued after `mr r3`), m0 declared before m1.
    - R30FBullJump: the landing tail (`MotionSetCore; MotionMoveF; xFE = 2`) repeated in the
      `pos.y < y` arm and the `else if (end)` arm (jump2 cross-jumps it); the call following the
      SndCall in one block makes its `li r3, 8` the last arg move like the target.
    - R10FGondola: one `f32 dist` for the tgt distance AND the `emsetNo % 3` limit switch (multi-set
      -> global pseudo f13 for the `fmadds` result), plus the GPR-first `cEmWepSetThrowF` alias
      (COMPILER-DIFF 1) for `w->pWep->setThrow(&spd, t, &Em10AtkTbl[5])`.
    - SetDamageDoor: switch-2's `case 1:` ends in `goto door_break;` into switch-1's case-2 body
      (`ble` target = the first type-check copy).
    - setHand: `bin = w->mot[9]; tpl = w->mot[14];` before the `wepType == 6` if in case 3 (tpl's
      shorter range -> r4, bin r9; 5 words left, see OPEN).
  - Fifth pass (374 -> 379, 2026-09-10; harness /tmp/em10e = em10d + `mini_run.sh`): global-alloc
    priority is `floor_log2(refs) * refs / live_length` with REG_N_REFS weighted by the LOOP DEPTH at
    flow time (a ref inside one loop counts 2, inside a `do { } while (0)` nested in it 3, ...), so
    loop notes are a lever that changes NO real insn:
    - `do { x = 1; } while (0);` around a store makes that set count one more per copy (SetDamageDoor:
      the three `in = 1` of the IN_CK macro -> `in` 13 weighted refs > `kind` 13/259, in r25/kind r24;
      the do-while around the WHOLE if-chain moved the arms' pool `lis` hoists instead).
    - Two nested `do { } while (0)` around `EmRoutineSet(..)` put its four `stb`s at depth 4
      (RackBreakCk: em 34 weighted refs > e 21/95 -> em r31, e r30). The loop notes are also a
      sched1 barrier: `int type = e->type;` BEFORE the notes and the ternary `type == 0 ? 0x3E :
      0x3D` inside keeps jump.c's hoisted `li 0x3D` behind the compare (the barrier insn's
      `reg_pending_sets_all` makes it output-dependent), so the loaded byte and the ternary temp
      share r0; with `int t = e->type; if (t == 0) t = ..` the temp is set by the insn where `e` dies
      and `expand_preferences` gives it e's r3/r11 preferences (t lands in r3, the hitCheck result
      moves to r0).
    - Source duplication that jump2 (which runs AFTER sched2 and global alloc here) cross-jumps
      away counts at allocation time: SetTakeawayPos writes `else if (bestAng < PI/2) { copy }
      else if (d < bestD) { copy }` instead of `||` in both loops (`&best` 17 refs > em 29/241 ->
      &best r28, em r27; the loop-2 `&c` PRE copy 10 refs > p -> r29); the final code is the `||`
      shape. LadderClimbCk is a `while (o)` loop with `o = o->next; continue;` written out before
      every `continue` (82 real insns at loop pass 2 instead of 70, so the Muku PI `lis` stays in
      the loop: threshold 71 * savings 1 * life 1 < insn_count); the seven copies cross-jump into one.
    - ShieldAtkCk: `f32 t = em->pos.y; d = pSUB->pos.y; d = t - d; d = fabsf(d);` — a multi-set `d`
      is not local-allocated, so `combine_regs` cannot tie the `fsubs` result to the dying `t`
      (`reg_qty[sreg] >= -1` -> fail) and it stays the op-2 pseudo `d`; the block-local `t` then
      takes f0 in local-alloc and global alloc gives `d` f13 (`fsubs f13, f0, f13`).
    - jump.c's `if (c) x = a; else x = b;` hoist (jump.c:479) skips the else-set only when the
      then-arm is not a single set or the else arm has REG_NOTES; a plain ternary is hoisted in
      jump1, so the `li` is free to schedule above the compare unless something depends on it.
  - OPEN (3): LostHead (59 words: `cmpwi cr4, r28, 3` in the switch dispatch block AND after `bl
    EmSetDie` in the shared case-0/1/2-else tail, the join's compare gone, cr4 saved with `mfcr r12`;
    our gcse never PREs it (single occurrence); `case 3: break;` makes it PRE'd but into block 2 + the
    case-1 body with the CC spilled through `mfcr r30`, 164 words; `case 3:` grouped with default or
    an `if (a == 3) asm("")` do nothing/worse — pass order (#5-like) rather than source); FindCk (20:
    the em3c bell OPEN — the target's pool `lis r10`/`lfs f9` of the 3-arm `r` and `fmuls r, r` are
    scheduled INSIDE the distance block and the high got r10 (r9/r11 busy), i.e. the identical arms
    were merged before local-alloc/sched1; ours cross-jumps them only in jump2, which runs after
    sched2 here, so the arm's `lis r9; lfs` stays first — if/else-if, `asm volatile("")`,
    do-while, inline expression and `rr` temp forms all 20); setHand (5: `no` r10 vs r11 with
    nothing else in r11, and the case-3 block order `lbz; lwz tpl; lwz bin; cmpwi` needs the
    compared byte NOT to die at the compare (an `asm volatile("" : : "r"(wt))` reproduces the order
    but moves the registers); switch/if-else/u8/u32 forms tried).

### Small tool RELs, third pass (t_camera_draw Matching; t_camera_data 12/16, t_movie/t_se_at 11/19; 2026-09)

- t_camera (include/t_camera.h): `TcWork* pTc` is a plain global pointer (`.data` 0x55C -> the static
  `tcWork`, 0x644); `cam` (a `Camera`) sits at +0x10, the pad snapshot `JOY joy` at +0x10C. The tool
  reads the pad words through raw offsets (`TC_TRG = *(u32*)((u8*)pTc + 0x120)`): only that reloads
  `pTc->joy.trg` after a store to the blink counter (a non-struct load aliases the scalar store) while
  the pointer itself is kept, and it keeps the `lwz trg` below `stw blink` at the function end.
  Data pools are globals `tcTypeTbl[64][16]`, `tcAdat[0x60]`, `tcCdat[0x40]`, `tcLdat[0x40]` (records
  = cam_ctrl.h's file records with the arrays inline: TcCdat has `u16 frame[26]` at 0x10, a
  `union { f32 floor; Vec dir; }` at 0x44, `pos/at[26]`, `roll/fovy[26]`), plus `Camera tcGameCamera`
  in t_camera_data. Data labels were named by hand (`sync_rel_symbols` mis-assigns them when the
  relocation order differs) — check `symbols.txt`/`sym_map.tsv` after every sync.
- A single `int num = ngon->num;` local (instead of re-reading the field in the loop test) is what
  keeps `ngon` out of a callee-saved register and orders the three `addi rX, r3, 4/8/0xc` bases
  (tcDrawNgon); a struct local `p.x = p.y = p.z = 0.0f; for (j) p.x += b[j]*px[j] ...` IS turned into
  register accumulators by our compiler (the zero stores stay, `fmr f9,f31` copies in the preheader).
- A `CameraBSpline* bs = &CamBSpline;` local (like cam_ctrl.cpp) makes `bs->k` a `0(r30)` load; the
  bare global gives `lis/lwz CamBSpline@l` for the offset-0 member. Declare such externs in the tool
  header, not cam_ctrl.h (a header extern before cam_ctrl.cpp's own would reorder its .bss).
- Two passes of loop.c (`-frerun-loop-opt`): pass 1 eliminates a `for (i...) { p = &tbl[i]; }` biv
  through a giv whose add_val is a *pointer-flagged* register (`maybe_eliminate_biv_1`,
  `REGNO_POINTER_FLAG`), and pass 2 then fails BCT ("Initial value not constant") -> pointer compare
  instead of `mtctr`. The flag came from re-using one function-scope `TcCdat* c` (also assigned in a
  later loop); a block-local `TcCdat* cd = &tcCdat[i]` per loop keeps `bdnz`. Dump with
  `cc1plus ... -dL` (the `.loop` file lists "insert_bct" / "biv eliminated" per loop).
- gcse PRE splits a loop counter into `addi rT, rI, 1` in the arms + `mr rI, rT` at the end when the
  increment sits at the latch of a body with an if-skip; the original has the plain `addi rI,rI,1`
  early in the body (tcDataExport rec loop: write `i++;` as the first statement after the header
  stores). The frames loop keeps `mulli r9, r6, 0x394` (no giv reduction) only with the for-header
  `i++`; `&tcCdat[i++]` reduces the giv.
- `(u8*)p - buf` written as an expression inside the loop (not an `ofs` variable) is the giv the
  original strength-reduces (`subf r10, r29, r3` in the preheader, `addi r10, 2` per iteration);
  a separate `ofs += 2` variable gives the same code but PRE hoists the `subf` above the previous
  loop. Per-section typed pointers (`Vec* vp` adat, `Vec* pos` cdat, `u16* fp` frames, `size =
  (u8*)fp - buf` before the rec loop) reproduce the `mr r5,r8 / mr r3,r5` section copies; the cdat
  loop's `pos[j] = cd->pos[j]` (index form) + `*at++/*roll++/*fovy++` + `pos = (Vec*) fovy` gives
  the `mr r6, r5` giv copy for pos only (pos is live on the skip path, the others are block-local).
- Compare constants: the tree folder turns `ver < 2` into `ver <= 1` (`cmpwi 1; bgt`) and
  `ver >= 2` into `> 1`; the original's `cmpwi 2; bge` shared with a later `ver <= 2` (cr0 saved in
  `mfcr r25`) needs the constant to reach RTL unfolded — `int lo = 2; if (ver < lo)` (cse then
  propagates it and gcse merges the two compares). `ver == -1 || ver < -1` are two separate ifs
  (`cmpwi -1; beq; blt`).
- `ver <= 2` / `ver > 3` version arms: `if (ver <= 2) {A} else {B}` gives `bgt` on the saved cr0;
  `ver <= 3` a hoisted `cmpwi cr4, r3, 3`.
- File-record pointer walks the original hoists into locals: `Vec* pt = s->points; for (j) a->pt[j] =
  *pt++;` (a plain `s->points[j]` reloads the pointer after every store because the stores may alias).
- t_se_at (src/t_movie/t_se_at.cpp, SE attack editor; work `SeAtWork` 0x16D4 = Debug_alloc'd, `TSeAt`
  = snd.h's SeAt with `blk`/`se_no` as ints): the work pointer and the current-area pointer are
  one-member structs (`seAtWk.p`, `seAtCur.p`); `static int seAtSaveNum` (its low half is read with
  `lhz seAtSaveNum+2` for the u16 header count); `SetToolLight(int)` is this module's db_light_v2
  copy (scope global in symbols.txt although the .sym says local). Table definitions sit right before
  the first function using them (menu strings in .rodata parse order: block names, ToolSeAt,
  seAtInit, main menu, seAtAreaEdit, create+edit menus, AreaMove pool, input/rnd/flag names,
  DataInput, DataLoad, save menu, DataSave). Idioms: `BitSet(w->save, TOOL_FLAG(..))` for the two
  flag backups (pG reloaded after the store); `int zero = 0` at the top of seAtInit (`li r28,0`
  before the first call, reused for every zero store); `u8 valid = w->copyValid` loaded once for the
  four menu-enable stores (order edit[4], edit[3], create[2], create[1] -> issued create[1] first);
  `Vec axis = {0,1,0}` declared inside the `if` (16-byte template copy); the `+-1/+-10 with X` steps
  are a macro, the clamps `if (v >= 0) { n = v; if (n > M) n = M; } else n = 0;` with a second
  variable (`mr r10, r11`), the s8 cursor clamps in DataLoad the ternary form (raw byte kept); the pad
  masks are `RIGHT|0x20000` / `LEFT|0x10000` (joy.h's SLEFT/SRIGHT swapped); `s16 x = w->x + 0x60`
  (`lhz/addi/extsh`); menu-name loops `for (j = 0; j < num; j++)` with `int num = 6` (a literal
  bound folds to `ble tbl+0x14`, the original has `blt tbl+0x18`), a `u8 col` for the loop colour
  and `(u8) col2` casts on an `int col2` in the footer (per-call `clrlwi`); colour ternaries inline
  in the eprintf argument (`yesNo == 0 ? 6 : 7` -> `li 7; bne; li 6`; nested
  `sub == 1 ? (cursor == 0 ? 6 : 0) : 0` for two separate `li r5, 0`); `pGS->stage_no` in the save
  path (keeps `lwz pG` below `sth w->y`). Open: ToolSeAt `lis` placement, seAtInit's Snd save/zero
  store order and the work-pointer load position, EditMenu's r7/r8 menu pointers (mine `mr r7, r8`
  on the edit call), AreaMove's `&right` address pseudo (y/z stored via `addi r9, r1, 8`),
  DataInput `clrlwi r5, r29, 24` per iteration in the first name loop, DataLoad's `(u8)` cast of the
  first footer colour (folded into the arms by our front end), DataSave's `seAtSaveNum` reload after
  the record copy.
- A `u8` counter the target does not constant-fold (`mr r3,rN` at uses, `clrlwi` only at a u8-param
  call) is an `int` incremented through call arguments (`f(nGen++, ...)` twice).
- `lwz r0,g; mr r7,r0; cmplw r0` around an early-return test = gcse PRE's copy: keep a single early
  return and read the global again inside the later block.
- `lbz rV,0(p) ... or r0,rV,r0; extsh rV,r0` = `s16 v = p[0]; v |= p[1] << 8;`.
- `addi rX,r1,ofs` for a block-local object hoisted above an earlier call = `T* p = &obj;` declared
  before the `if`.
- 8-byte zero-initialised `static char name[8]` sits in `.sdata`; 10 bytes would move it to `.data`.
- gcse PRE hash: hash = 119+6+(61<<7)+h(name), h = h*129+c, table = n_insns/2|1 buckets; n_insns at
  gcse time excludes cse-deleted dead initialisers; `asm volatile("")` before a loop disables its
  invariant motion (exception `ErrorHandler`, t_bugcheck).

- Tools REL, fourth pass (t_motseq 15/21 written, src/Tools/t_motseq.cpp; t_rck 13/33 written, src/Tools/t_rck.cpp;
  neither Matching; include/db_mod.h grew `DbModSlot::seqFlag` (0x148) and the dbModSetViewFlag/UnsetViewFlag/
  MotionSetSeq(int, void*, u16, u16)/GetMotFilename declarations plus the `dbModMotionSetSeqI` u32-argument alias
  (COMPILER-DIFF 4), 2026-09):
  - t_motseq: work = `MsqWork` (0x1E10, `MsqSeq seq[1]` of 0x10C0 = `u16 num; u8 reverse; MotionSeqKey key[1024]` +
    editor bytes, then mode/sub1..3, a JOY copy at 0x10D8, the eprintf colour byte 0x15B8 with an UNALIGNED
    `GXColor bg` at 0x15B9 (`lwz 0x15b9` for the by-value GXSetCopyClear), seqNo/fileName at 0x1CA4/0x1CA8,
    start/end/add/max 0x1DAC..0x1DB8, seMode 0x1DBC) behind the one-member struct `msqWork.p` (.data, `= {0}`);
    routine table + `int msq_y_tbl[21]` follow it in .data. `msqSetMode(m)` = mode, sub1 = sub2 = sub3 = 0 as
    four plain statements (each reloads the pointer, so the order is the source order).
  - The Debug_alloc store with `lis rX,work@ha` before the call: `MsqWork*& wp = msqWork.p; wp = Debug_alloc(..)`.
    Pool order 0.0 before 1000/3000 while the first store is a 1000: `f32 zero; zero = 0.0f;` assigned right
    before the block and stored through the variable (the constant enters the pool at the assignment).
  - `strcpy(p, "0.seq")`-style byte stores written out (`p[0]='0'; ... p[5]=0;` natural order gives the target's
    `stb 5 first` schedule; the known-zero `sub1` register is stored for the terminator).
  - A case body that is a fall-through target of another case in the target (`case 5: bl msqSaveFile; b case7`) is
    TWO full copies in the source (case 5 with its own tail): the later copy keeps the known-zero register of the
    switch region; a source-level fallthrough label has two predecessors and gets fresh `li 0`s.
  - Default arm written first (`case 0: default:`) lays it out right after the compare tree; identical case bodies
    cross-jump into the LAST copy (4/6 shared tail sits after case 6, so source order 4, 5, 6, 7).
  - msqDisp: the flag/SE display block exists twice in the source (hand-inlined, strings once); its `int cx = 3`
    is declared right before the "--SEQUENCE INFO--" eprintf so cse folds `cx*8` to 24 for the lines before the
    Free loop and the SE line after the loop keeps `slwi r3,r17,3`; the loop y is the giv `238 + i * 14`.
    Palette `GXColor c1/c2/c3 = {..}` locals are 4-byte ADDRESSOF slots (`stw 0; stb` init), `col` is a fourth.
    `const f32` step constants (5, 4, 24, 25, 15) are declared right before the grid loop (after `x = 40; rc.w = 13`)
    to get the target's pool order with 369 last.
  - QuitCk: `cmpwi 0; beq L; cmpwi 1; L:` (dead second compare) is a switch whose case 1 shares the default body
    (`case 0: S; case 1: default: S;`); ours merges the bodies fully and drops both compares (open, -8 bytes).
    Sequence's lone dead `cmpwi r30,0` after the frame clamp is open too (no if/switch/dead-store form keeps it).
  - SeqResize (open): the target's delete loop is un-rotated (`b LOOP` back edge) with a PRE'd `num - 1`
    (`subi r0,r7,1` in both predecessors) and a real `lhz r7` reload after the `sth` — every while/for/do form
    tried rotates the loop or forwards the store to the load (`clrlwi`).
  - t_rck: `RckWork` (0x14AD4: mode/step/cursors/flags, `f32 curX, curY` used as a Vec, camMode, JOY copy at 0x28,
    screen centre floats 0x290.., catchTimer/cur/near/lineStart/savedRtp, `RckHeader hdr` (magic "2RTP", nPoint,
    nLine, nSq = nPoint², ofsLine, ofsNext), `RckPoint pt[128]` (Vec + lineOfs + nLine), `RckLine line[128][128]`
    (s16 to, u16 len), `s8 next[128][128]`) and the 0x14818 save buffer, both behind one-member .bss structs;
    rckSetNextPoint is Dijkstra over a 0x400-byte `RckNode[128]` frame array; the mode table is a local
    `void (*tbl[6])()` template copied to the stack in ToolRctRouteCheck; `.data` ends with `.balign 8`.
  - Screen-position conversions in the eprintf2 labels are `(u32)` (fcmpu 2^31/cror/bso pattern), the point
    height snap is `(f32)(s8)((y + 62.5f) / 500.0f) * 500.0f` (`psq_st qr4` + `extsb`).

### Stage rooms, third pass (cSceObj.cpp 17/18; r220, r40c, r20a Matching; r40f 8/9, r218 5/8, 2026-09)

- `src/st/cSceObj.cpp` + `include/cSceObj.h` (0xF8-byte mover, no vtable: `move` dispatches through a
  *local* `int (cSceObj::*tbl[3])()` copied from its `.rodata` template; a class without virtuals makes
  g++ 2.95 call the PMF without the index test). `accFrame/cstFrame/decFrame/cnt/frame` are `u32`
  (2^52 magic without `xoris`, `fixuns_truncsfsi2` with the 2^31 compare). Idioms found there:
  - `MTX_COPY` inside an inlined member: `MtxPtr d_ = (dst); MtxPtr s_ = (src);` (both initialised at
    the declaration) gives the target's `addi r7,s; addi r8,d` giv registers; sce_at.cpp's
    `d_ = (dst)` assignment form swaps them here.
  - Inline helpers whose frame is one `Vec d`: `&d` is the inline frame base (`(plus fp N)` at offset
    0), so `PSVECSubtract(target, .., &d)` gets `addi r5,r1,N; mr r27,r5` (arg first, copy after) while a
    hand-written `Vec d` local of the caller precomputes a pseudo (`addi r27; mr r5,r27`).
  - A second `if (obj)` block right after a first one is jump-threaded (`beq` straight to the end)
    unless something sits between the label and the compare: `cModel* o = obj; if (o) {...}` in the
    second block (cSceObj moveTo) keeps the target's `beq` to the second test AND stops gcse from
    PRE-copying `&d` (`mr r25,r27`) — the same frame slot `(plus r31,0x28)` is one gcse expression
    across `case 0`'s `Vec pos` and `case 6`'s inline `Vec d`.
  - `step = mode = 0;` (chain) then `frame = n;` gives `stb 0; stw 1c; stb 2` (setMove1_all); separate
    statements put `stb 2` before `stb 0`.
  - The in-class ctor zeroes 46 floats: source order = member order but `srcPos, dstPos, srcRot,
    dstRot` (src before dst); the target's `stfs` stream is RTL order except that the last (dying)
    store jumps forward to where haifa's pending-memory flush (32 refs) splits the block (18th in
    r220, 25th in r40c) — no source change needed, ours reproduces it. The `sub[i] = NULL` loop is
    the LAST statement (forward `bdnz` with `u32 i`, the `mtctr` set up among the stores).
  - OPEN: setMove1_all issues `lfs f0,0.01` before the two `Vec` struct copies (target after them,
    `lis` stays early); 20 source forms (chains, FSet, locals, do-while, copies first) tried.
- Rooms with a stack `cSceObj` (r220 moveElevator, r40c): `int evt = 1;` declared BEFORE the
  `cSceObj elv;` is compared after the ctor's loop label → survives as `li r0,1; cmpwi cr3,r0,1` (cse
  cannot see across the label, cprop cannot substitute into `cmpwi`); `int up = dir == 0;` there is
  `subfic; adde`. A loop counter reused by two loops (the `sub[]` search and the 90-frame loop) is
  globally allocated (r31): give the call-free search its own `u32 n` (r11).
- Range tests: `type >= 0x13 && type <= 0x14 && lid && box` folds to `subi/cmplwi 1`; `if (type <=
  0x14) if (type >= 0x13)` gets `cmpwi 0x12; ble` (fold's `X >= C` → `X > C-1`); the target's
  `cmpwi cr2,0x14; bgt; cmpwi 0x13; blt` is a `switch (type) { case 0x13: case 0x14: ... }` (cr2 kept
  for the second switch on the same value). Two zero-tests of one flag word (`(f & A) == 0 && (f & B)
  == 0`) fold to one mask: `static inline u32 flagBit(u32 f, u32 bit) { return f & bit; }` and
  `flagBit(f, A) == 0 && flagBit(f, B) == 0` keeps the two `andi./andis.` (r220 initElevator).
- `if (SceMesGetSelection() != 1) { Comeback; SceEventEnd; SceExit; } else {...}` lays the fail arm
  out first (r40f). `while (em->ckGoto()) SceSleep(1);` = `b test; L: bl SceSleep; test: ...; li r3,1;
  bne L`. Countdown `for (i = 0; i < 20; i++) SceSleep(1);` = `li 0x14; L: ..; subic.; bne`.
- `SceMesCamSndSet` is called by the st2/st4 rooms with a 4th argument (`li r6,4`) the DOL's
  3-parameter definition never reads: `SceMesCamSndSet4(...) asm("SceMesCamSndSet")` in st_room.h.
- TexRender object setup (r20a): `x136 = 2; x137 = 8; x138 = 0x20; alpha = 0.7f;` with alpha LAST
  gives the target's `stfs alpha; stb 136; stb 137; stb 138` (the dying last store goes first).
- `const f32 ry = PI;` in the mid-block declarations plus `f32 y = ry;` right before the call whose
  successor stores it: the `lis PI@ha` lands at the function top (r18) and `lfs f31` before the call
  (r20a CarryOnShoulder); `f32 ry = PI;` alone loads at its declaration, `const` alone folds to a
  literal at the use. `Vec* pa = &ang; pa->y = y;` with `ang.x/z` direct gives `stfs f31,4(r21)`.
- Template-copied Vecs and a `Vec rot = {0,0,0}; rot.y = -PI;` declared mid-block after the
  `setNoSuspend` calls (frame slots still in declaration order: the whole declaration group moves).
- cEmDoor `setCloseLock`: the room-side `cEmDoorSetCloseLock(cEm*) asm("setCloseLock__7cEmDoori")`
  alias (r101/r105/r411) is needed in r20a too. `getRoomEtcDoor(no, &work->door, 1) == 0` then
  `work->door = NULL` stores r3 (cse knows it is 0).
- Death bits of the loaded enemy list: `int list = pG->emlist_no; if (list >= 0) v = *(u32*)((list <<
  5) + (u32) pG + 0x501C) & (0x80000000 >> (no & 31)); else v = 0;` (r218, like r101's emDeadClear);
  `while (1) { if (a) { if (b) break; } SceSleep(1); }` stays un-rotated (the `while (!(a && b))`
  form is rotated).
- OPEN (r40f BombSet): the first of two `Vec p = {..}` template copies loads its words 0,4,8 in the
  target and 0,8,4 in ours (the r10c/r22a "second word pair" OPEN item; the second copy is 8-then-4
  in both). OPEN (r218, = r108 openCover tail): after each `do { pos.y += spd; if (..) break;
  SceSleep(1); } while (1)` the target re-materialises `lis work@ha` and the 2500.0 constant into
  fresh registers where ours reuses the loop's hoisted r28/f31; `cObj* o28/o29` get r31/r30 swapped.

### t_esp window system (db_window Matching, db_widget 94/113, include/db_widget.h; 2026-09)

- Unit pins of t_esp were one string too late: every header-string group of the module starts with
  atari.h's `cFlag.set()` message, so db_port/db_widget/db_window/t_esp start 0x24 earlier
  (0x23D0/0x2CA8/0x3410/0x35E0). The seven implicit destructors `~DB_WINDOW..~DB_SLIDEBAR` that sat at
  the start of "db_window" are db_widget.o's linkonce tail (after its cManager<cLight> block):
  db_window.o begins at `DB_MOUSE::DB_MOUSE`.
- Implicit (synthesized) destructors of a class whose vtable a unit emits: the original compiler
  synthesized them from the vtable entries alone; ours only synthesizes at a use inside a function.
  A never-called `static` function that `delete`s a pointer of each class (dead-stripped, listed in
  STRIP_UNUSED) makes ours emit the same seven bodies (no vptr store, size 0x20/0x44), in class
  declaration order, after the template block. In-class `virtual ~X() {}` bodies are wrong (they
  store the vptr, +0x10 each).
- GNU v2 vtable order = virtual declaration order with the dtor where declared; the key method is
  the first non-inline virtual in TYPE_METHODS order (ctor, dtor, then declaration order), so a
  class whose dtor is implicit gets its vtable where its first user-declared virtual is defined.
- Grouped `case A: case B:` labels with one body give a range-folded compare tree; the target's
  per-value `beq` nodes with cross-jumped bodies mean each case had its OWN identical body
  (`case S8: *(s8*)p = v; break; case U8: *(u8*)p = v; break;` compile identically and merge).
- A parameter list the .sym cannot show: `DB_WINDOW::CallActiveChangeCallback` takes
  `(DB_PRIMITIVE*, DB_KEYBORD*)` it never reads (the caller's `mr r5, r30` is the only evidence).
- `id = (counter += 0x10)` in C++ re-reads the lvalue: `stw r9,counter; stw r9,id` then later
  `lwz counter; stw id` (two stores of `id`).
- `size.y = h; rect = DB_RECT(base.x, base.y, w, size.y); size.x = w;` — the reload of the
  just-stored member is forwarded as `fmr f12,f2` while the untouched parameter is used directly;
  the temp of a 4-arg inline ctor is stored through its `this` pseudo (`0xc(r9)`) and the first
  member through the frame (`0x8(r1)`).
- `!(a & 1) && !(a & 0x10)` is folded to `(a & 0x11) == 0` by fold_truthop; nested `if`s (or
  `(a & bit) == 0` forms) keep two `andi.`. `int ok = (flags & 1) == 0; if (ok)` gives the
  `xori; andi.; beq` first test of DB_NUMERIC::Update.
- A `DB_PRIM_ARRAY::ChkMouseButton(DB_PRIMITIVE*, DB_MOUSE m, int)` by-value class parameter
  (class with a ctor) is copied by the caller in 0x18-byte `lwz/stw` chunks and passed by address.
- Store-order shapes seen here: `a = b = g = r = 1.0f` (chain) stores r, a, g, b; a DB_WINDOW
  `DB_WINDOW* w = 0;` dead initializer is the zero register of the two preceding member stores;
  `for (j = i; j != 0; j--) win[j] = win[j-1]` (u32) gives `mtctr i` after an `i != 0` test where a
  do/while or signed loop keeps `subic.`.
- Open (db_widget, sections byte-equal, 19 functions): DB_PRIMITIVE ctor's 60-store block order;
  the seven `SetNumPointer` min/max stores (pool order 127,-128 but stores `a0` then `a4` after
  the int stores — no statement/const-local order gives both); `&base`/`&size` kept in
  callee-saved registers in DB_STRING::Draw / DB_WINDOW_TITLE::Draw (base.y via `4(r30)`, base.x
  via `0x3c(r31)`: a PRE'd `(plus this 0x3c)`); DB_STRING ctor store schedule (vptr store between
  the colour chain and `str = len = 0`); DB_NUMERIC2::OnCalcMsg keeps MIN/MAX cross-jumped and
  DEFAULT separate (ours merges all three, COMPILER-DIFF #6 shape); AddPrimitive (-8),
  CallActiveChangeCallback (`mr r9,r3` copy of this), DB_WINDOW ctor temp loads (`0xc(r1)` vs
  `0xc(r9)`), DB_NUMERIC ctor (-4).

### DOL structural-gap pass (main_mem 23->29/33, debug 4->5/11, sce_com 22->23/33; 2026-09)

- main_mem `MemCheckUsedHeap` (0x1290, was unwritten, now 95.7%): the heap tile display. Tiles are a
  local 0x20-byte `MemTile` (libgpu's TILE is 0x18), `static MemTile tile[2]` + `static int ey_base = 30`
  are the `tile.579`/`ey_base.580` statics, the `lbl_80314BF4` zero word is a `static int = 0` inside the
  dead-stripped `memSetCheck()` that owns the trailing "memset error: " string (unit now in STRIP_UNUSED),
  and the 4-byte `.bss` tail is main_sub's 8-alignment (`asm(".section .bss; .balign 8")`). The function
  is `extern "C"` (main_mem.h). Idioms: ONE `MemTile* mt` for the loop tiles and both static tiles (a
  reassigned pointer is global-allocated into r31; separate locals fold into r4); `tag` block-local per
  cell loop; `char* p = NULL; char* buf = p;` (one zero register stored to the spill slot); the clamp
  through a temp `n` with `ISet(ey_base, n); ey = ey_base;` (reference store, forwarded `mr`);
  `if (cnt++ == 0x1FF) full = 1;`; the `pSys->flags` progressive test must be a reference read
  (`SysRef(pSys)->flags`, mercenaries.cpp idiom) or sched1 hoists the fixed-scalar load above the tile's
  halfword stores and issues the byte stores first (they sit on the load's dependence path). Same in
  debug processBarDisp/PrimitiveBuffDisp. Open: `li`/`stw code` issue order inside the tile blocks, the
  fpmem `mr` copies of the end-marker block, y1/tag temporaries r8/r10.
- Index register class, refined (main_mem MemSignalHeap, now 100%): `add rD, rIdx, rPtr` (index first)
  with the index in r0 = integer arithmetic on a pointer-typed LOCAL: `OSHeapDescriptor* hh = HeapHead;
  *(T*)(h * sizeof(T) + (u32) hh)` — expand_decl flags the local's pseudo as a pointer (regclass then
  makes the other operand an index, GENERAL_REGS), while a global pointer loaded into a temporary is not
  flagged (`heap_backup[h] = HeapHead[h]` in MemSuspendHeap keeps the offset in BASE_REGS r10).
  MemReplaceHeap's `add r11, r11, r0` (pointer first, offset r0) is still open (no local/cast form found).
- SystemMemInit: all twelve SysMem constants are stored BEFORE `OSGetArenaLo()` (usb/debug included,
  two spill into r29/r30); the store order is field order with `heap_end` first.
- Two zero registers for `sth` stores of the same value (debug processBarDisp/PrimitiveBuffDisp tile 0
  vs later tiles): the first `t->z0 = 0` is a HImode constant (own `li r0,0`), the later stores read an
  `int z = 0` declared AFTER that store and before the block's call (SImode pseudo hoisted into a
  callee-saved register); a HI constant store cannot reuse a later SI zero, and an earlier SI zero would
  have been reused by the first store.
- debug processBarDisp (61% -> 91%): bars are y/1.3333334f + 56 and h/1.3333334f in progressive mode
  (not /2 + 32), the tick base is `OS_BUS_CLOCK / 240 * vcnt` (`mulhwu 0x88888889; srwi 7`), the time
  values are `(f32) tick * 60.0f / (f32) (OS_BUS_CLOCK >> 2) * 100.0f`, the `g_proc_cnt = (u32)` of the
  same, the last two tiles are the 400-line background bars (x 6 / 12, colour 8,8,0x20), the loop is
  `for (i = 5; i < idx_bak + 5; i++)` printing `proc_tick[i] - proc_tick[i - 1]`, `cnt = (cnt + 1) & 3`
  before `xchr[cnt]`. A member read back right after its store (`t->h = x1 - x0; PROG_H(t->h)`) is the
  target's `sth r9; mr r7, r9` copy; a local gives none. PrimitiveBuffDisp (61% -> 88%): the sum is
  `(f32)(int)(...)` (signed magic only), `if (pb->rate < rate)`, `s16 width = 200` converted with
  `psq_l qr5` (the target's extra 200.0f pool word is still missing), `t = tile` assigned after `rate`.
- sce_com SceChapterEnd (87.7% -> 99.3%): FadeSetW for all four fades; `U16Set/U8Set` for every pG store
  in the door block and `U32Set` for the counters, with `U16Zero(u16&)` (`d = 0` inside the inline) for
  `x8338` so its HImode zero gets its own `li` and the pG reload; `GameSaveSave(&GameSave, pSaveData, 2)`
  (r5 = 2); `len = len + 0xC; len = len + MARGIN;` (fold reassociates the one-liner); `sel = 0` after the
  `SceSleep(1)`, `x4F9E = 0; room = 0;` right before `SwapOut`; `room = pG->room_id` before `x4F9E =`;
  `memcpy((u8*) pPL + 0x94/0xA0, &plPos/&plRot, 12)` for the restore (pPL reloaded between). Remaining:
  the `zero`/`&col` pseudo pair r26/r27 swap and x4F9E/room r24/r25.
- sce_com: `0x150 - cMes.getWork()->lineSpace - cMes.getWork()->fontH - 1` (lineSpace first) in the
  DOL's own object too (SceMesCamSndSet now identical). OpenBoxMain's per-frame steps are folded
  divisions (`1.9198622f / 30.0f` = 0x3d831006, `500.0f / 30.0f` = 0x41855555; the decimal literals
  are one ulp off) and its pool lists the totals before the steps and `-0.0349/-0.01745` last — the
  source order that produces that pool is still open (the code is 96%).
- An `extern T alias asm("sym")` view struct must be larger than 8 bytes (pad) or the alias lands in
  small data (`lwz r10, Dvd@sda21`).
- Tool: /tmp-style masked byte compare with difflib alignment when function sizes differ (relocs masked
  on both sides, same-section branches resolved by target) is the metric to brute-force statement orders
  with; position-based word compares are dominated by the shift after the first length change.
- Menu enable flags from one value: chain `a.enable = b.enable = c.enable = d.enable = valid;`
  reproduces the store order and base assignment; separate statements do not.
- Clamp-then-store: `n = clamp(n); p->f = n;` reloads the pointer after the ternary; `p->f = ternary`
  hoists the pointer loads above the clamp (LHS evaluated first).
- A variable reused across `switch` cases keeps its register; a separate `n` shifts the others.
- `if (a & 2) if (b & 2)` must be nested; `&&` on adjacent bytes folds into one `lwz`+`andis.`.
- Loop with an exit flag: `if (hit) break;` + post-loop `if (hit) {..} else {..}` keeps the pool `lis`
  out of the loop body (jump threading removes the post-loop test).
- Both arms of a mode select spelled out (same tails) cross-jump into one block using the `andi.`
  zero register; a single tail after the `if` starts a fresh `li 0`/`lis`.
- A local function-pointer table initialised inside the function is a `.rodata` table copied to the
  stack at entry.


### Enemy/system DOL units, bytes-first pass (scroll, at_sub Matching; shadow 33/36, em_set 10/12, item 63/78, snd 91/97, emmine 19/23; 2026-09)

- Judge with a masked byte compare that (1) strips the `.NNN` DECL_UID suffix of static locals and the
  `_8019XXXX` address suffix of `__static_initialization_and_destruction_0`, (2) compares data-section
  reloc targets by *masked* content (a `const char*` table's relocated words are 0 in the split object and
  the string offset in ours) or by global name, and (3) treats an undefined symbol as a data target, or a
  `static const Vec ofs` / weak `_vt.5cUnit` copy / `kind_str.658` table shows up as a false diff.
- COMPILER-DIFF 5 shape, per site: `mr r3, obj; lwz r0, 0(r3); andi.; beq` at the top of a manager-walk
  body (the argument copy hoisted above the flag test and the load rebased on r3) is `cObj* o = obj;
  asm("" : "+r"(o));` before the test with `o` used for both the test and the call (shadow ShadowTrans
  em loop, shadowScrModelRender obj loop). The *other* loop of each function keeps `lwz 0(r31)` first,
  so the launder must go only where the target shows the hoist.
- A unit static read twice around a scalar-reference store (`dir.x = g; FSet(dir.y, -1); dir.z = g`):
  `FRef(g)` on both reads (reference read, neither in-struct nor scalar) issues the pool `-1.0` load
  before the `.sdata` loads like the target; the plain reads put `lfs g@sda21` first (shadow
  make_comn_parallel_light).
- A pool `0.0` that sits before the block's other constants but whose `lis/lfs/stfs` are issued after
  them: `const f32 zero = 0.0f;` declared before the other statements (pool entry at the declaration)
  and `rot.z = zero;` written *last* (make_comn_parallel_light `rot`).
- `SoftShadowGXDraw(mng, u32 div, f32 x, y, z, u, v, alpha, scale)`: the target issues the x/y/z
  moves before `li r4, div` — COMPILER-DIFF 1, floats-first alias
  `SoftShadowGXDrawF(mng, x, y, z, div, u, v, alpha, scale) asm("SoftShadowGXDraw")` (div after z is
  the only position of five that matches all five call sites).
- A float local whose `lfs` the target issues after the first call while the other callee-saved FPRs
  (`z` f29, `alpha` f30) come first: declare it without initialiser and assign `zero = 0.0f;` after the
  call (MakeSoftShadow). OPEN there: the target loads the constant into f28 and copies `fmr f31, f28`
  twice (after SetNoScissor and in the else arm); every zero-variable/inline/literal form merges the copies.
- Local-alloc `fake lifetime` (local-alloc.c: each qty is first tried with birth-2/death+2 so that
  adjacent qtys get different registers): `lwz r11, pSnd; lwz r9, 0x9c(r11)` is the normal result;
  the target's `lwz r9, pSnd; lwz r9, 0x9c(r9)` (snd SndSetReverb then-arm) means one of the two qtys
  was allocated on its real lifetime (a suggested register) — OPEN, no source form found.
- sndFilterCalc: `no = h->filter_ofs[no]` (the offset reuses the `int no` parameter) makes the loop
  bound `num` land in r0 and `ret` in r8; a separate `u32 ofs` local gets r0 itself and pushes `num`
  to r8 / `ret` to r7. Reusing a dead parameter for a derived value is a cheap register-order lever.
- snd `.sdata` is 8-aligned in the split object (`u8 flag_bak` + 7 pad): `asm(".section .sdata; .balign 8")`.
- scroll idioms (unit Matching): `cObj** p = (cObj**) (i * sizeof(cObj*) + (u32) scrObjTbl); *p = NULL`
  (index-first integer address, store through a plain pointer) reloads the scalar `scrObjTbl` every
  iteration and keeps `li r10, 0` in the preheader (SmdClear); a `pLog->err(0, 0, "fmt", id)` whose
  format string is a `const char* s` local expands the string address before the pLog load, which
  stops jump.c from hoisting a preceding `return NULL` into `li r3, 0; beq` (the three `li r3, 0` tails
  are cross-jumped instead) and gives each error block its own lis/lwz schedule (SmdGetObjPtr,
  SmdGetGroupObjPtr); the shared NULL return placed after the second error call with the call jumping
  over it is `pLog->err(..); goto ok; ng: return NULL; } ok: return scrObjTbl[id];` (SmdGetGroupObjPtr);
  `if (p) { do { cur = p; next = cur->next; p = next; ... } while (next); }` for the list walk that
  tests `next` at the bottom (BlockDestroy); `nTpl = 0` written *before* the `getWorkPtr(0)` call so
  the pseudo crosses it and takes r30; `u32 base = (u32) this + ofsBin; ((u32*) base)[i]` (an integer
  base, not a pointer) ranks below the loop-hoisted `0x02FFFFFF` constant in the callee-saved order
  (slide); the dead `SmdGetIdNumPtr` needs STRIP_UNUSED.
- em_set: a plain `for (i = 0; i < EmMgr.nArray; i++) { cEm* em = emSetWork(i); ... }` (the inline
  reading the bound through a manager copy `cEmMgr* m = &EmMgr`) gives the target's `lwz r0, nArray;
  cmplw; bge` entry test, the PRE copy `mr r10, r0` for the in-loop range check and the back edge
  threaded past that check (checkListId, GetEmPtrFromList) — the guarded do-while form kept a separate
  bound register. `d->id` read directly at `== 0`, in `EM_SET_ID_NG` (range fold `id - 3 <= 1` in
  QImode) and as the `EmCreate` argument: the QImode load stays and the int uses share one PRE'd
  `clrlwi r4, r11, 24`; a `u8 id` local is promoted and never masked (EmSetFromList).
- item: a work pointer assigned in several `switch` arms is ONE function-scope `ItemWork* p` (case 3's
  `search(0x3E)` result lands in r31); per-arm `ItemWork* p` locals give r30 (set_char, set_stage2).
- at_sub: `d = SQ_DIST(c, p0); if (d < rr) ...; d = SQ_DIST(c, p1); if (d < rr) ...` — the twice-set
  `f32 d` is not local-alloc'd, so the final `fmadds` result is not tied to the dying dz operand and
  global alloc gives it f0 (dy f0, dx f13, dz f12); inline expressions tie the result to dz
  (AtSphereCapsuleCk). At_poly_sphere_ck2: `f32 ad = fabsf(d); hit = 99; if (ad > r)` keeps the
  `li r26, 0x63` after the volatile asm; `hit = 0` written inside the surface-hit arm (with an
  `else hit = 0`), `if (i == 3) hit = 0` inside the `!(flag & 0x20)` arm, and the rim-hit arm's
  `hit = 1` as `goto hit1;` into the surface arm's `hit1: hit = 1;` — the original's cross-jump
  survivor is the FIRST copy (COMPILER-DIFF 6 shape avoided by the goto).
- emmine: the target never compares `eff0[0] == 0xD2 && eff0[1] == 1` in emMine_R1_ShotArrow (a check
  copied from another unit); setBomb's tail is `xFF = 0; xFC = 1; xFD = 6; xFE = 0` (ff first). OPEN
  there: `stb r27, 0xfe` stores the `hit` register (known 0, cse canonical for the QI zero while xFF
  keeps the HI zero r26) and the EstSet stack zeros are stored 8 then 0xc; ShotArrow's allocation
  (target w r31, attr/info share r30 so the two water blocks cross-jump, em r29, &hit r28) and
  R1_Fall's spill-slot rotation (0x100/0x104/0x108) are still open.
- OPEN, register only: make_comn_fit_light / make_comn_parallel_light `lis r11` vs `lis r10` for the
  1.0 pool high (the fpmem loadaddr pseudo, 4 refs, out-ranks the 2-ref `lis` in local-alloc);
  sndVolCalcSub `r` in f2 with `vol` copied to f9; item init `li r3, 0x20` before `addi r4, str`;
  set_stage2 cases 2/3 `mr r3, this` before `li r4, id` at the block start; em_set EmSetFromList2 /
  EmSetEvent (`&em->pos`/`&em->oldPos` both callee-saved, the EmSetWork constants' lis/lfs
  interleave); em_sub EmCatchMotionMove (`step` tied to the dying `rate` by local-alloc in ours,
  separate f13 in the target); motion MotionSequenceCtrl (f11/f12/f13 permutation); snd
  SndRoomBgmStart (seq r31 / no r29 / w r28 in the target).

### DOL UI/camera/scenario pass (cam_ctrl 66->74/84, cam_qfps 19->21/24, route_ck 9->10/16, sce_com
23->24/33 (.rodata now equal), debug 3->5/11 (.rodata now equal), t_option 19->21/22, pl_wep 24/29; 2026-09)

- Two independent stores through ONE base register (`stw flags; stfs ratio` on `CamSmth`): sched1's
  tie-break prefers the insn with the smaller register weight, and the base register's REG_DEAD
  sits on its LAST user in RTL order, so the store written last in the source carries weight -2 and
  is issued FIRST. cam_ctrl `smoothStart`: `u32 f = CamSmth.flags; FSet(CamSmth.ratio, r);
  CamSmth.flags = f | 1;` gives `lwz flags; lfs ratio; ori; stw flags; stfs ratio` (the FSet keeps the
  ratio load below the flags load, the split RMW makes the flags store the last user of the base).
  Same lever in pl_wep PlSetLockPitch (`m3r[2] = 0; m3r[1] = p; m3r[0] = ...` in a `do {} while (0)`:
  the loop notes keep the `lis` pair below the preceding `fmuls`).
- `fmuls fD, fA, fD` (variable operand second): `x = a * x` is expanded with the destination
  swapped to operand 0 (optabs `expand_binop` puts a target that equals op1 first), so the target's
  order means a THIRD variable (`prod = s * dot`) that local-alloc tied to `dot`'s register — which
  needs `dot` block-local with ONE death: the earlier `dot = PSVECDotProduct(); s = dot / mag` reuse
  of the same name must become its own local (`dot0`) (cam_ctrl searchRail).
- `ver2 = 0` assigned after an early `return` (instead of `int ver2 = 0` at the top) leaves the `li`
  after the following `strncmp` call (cam_ctrl calcAddr); `int cur = 2` / `int zero = 0` declared at
  the top of the `if` block whose calls precede the store (t_option tp_pl_posmove / tp_pl_weapon:
  the constant lives in a callee-saved register across TaskSuspend/TaskSignal, `setRno(.., zero)`).
- COMPILER-DIFF 1 alias: `de_Boor_CoxF(int n, f32* knot, f32 t, int k, f32* out) asm("de_Boor_Cox")`
  (math_sub.h) for cam_ctrl BSpline's `lfs f1` before the `lwz r5`/`addi r6` argument moves; the GPR
  arguments keep their order in the alias (ABI-identical), only the float moves earlier.
- COMPILER-DIFF 2 launder for a `u8` switch value: `int st = x36; asm("" : "+r"(st)); switch ((u8) st)
  ... x36 = st + 1` reproduces `lbz r11; clrlwi r0,r11,24; cmpwi r0` with the raw byte kept for the
  increment (cam_ctrl r0_Free; `char st` gives `extsb`, `u8 st` no mask).
- `pGS->debug_mode` (struct view of pG) after Vec/struct copies keeps the `lwz pG` below the copy's
  loads/stores (cam_ctrl r0_Track/r0_RailPan, cam_qfps hitCheck); `FSet(angle_y, clamp)` (scalar
  reference store) keeps the following `lfs C_RANGE` (fixed scalar) below it — the target loads the
  static after the Key byte load, ours hoisted it (cam_qfps calcDepressionRatio).
- `qfps.a(); if (...) qfps.b(); qfps.c()` chains: the member reads and the calls inside the `if`
  arms through a local `CameraQuasiFPS* q = &qfps;`, the join-block calls on `qfps.` directly, give
  the target's `addi r30,this,0x278; lwz 0x148(r30); mr r29,r30` PRE copy pair (cam_ctrl
  switchCamera case 8).
- `param.roll *= ratio; param.roll = p->roll * (1 - ratio) + param.roll; param.fovy *= ...` (one
  member fully updated before the next) is the target's interleave; two `*=` then two sums is not
  (cam_ctrl CameraSmooth::move).
- Zeroing a POD `Vec` local in one arm (`memset` libcall with `crclr cr1eq`, `&p` PRE'd with the other
  arm's copy and kept in a callee-saved register across the call): `static inline void vecClear(Vec& v)
  { memset_v(&v, 0, sizeof(Vec)); }` with `extern "C" void* memset_v(...) asm("memset")` — the `(...)`
  prototype gives the varargs `crclr`, the inline's reference gives the pseudo. `p = Vec()` gives a
  zeroed temporary plus a block copy, a prototyped `memset(&p, ...)` no `crclr`, two block-local
  `Vec p` per arm no shared tail (route_ck RouteCkGetPoint).
- `(RtpPoint*)(next * 16 + (u32) pts)` copies where the target's `stw` tail is cross-jumped into the
  function's last copy (1-insn match against the return label's predecessor) need the same
  registers in both copies: `u32 base = (u32) rtpPoint(rtp); pts = (RtpPoint*) base;` (an integer
  base for the index arithmetic) gives the `slwi r9; lwzx r10,r9,r4; add r11,r9,r4` register set
  (route_ck RouteCkPosToPos).
- `va_list ap; va_start(ap, fmt); char buf[N];` — the buffer declared AFTER `va_start` (C++
  mid-block) puts the register save area and `ap` below it (target frame: saves 0x8, ap 0x80, buf
  0x90); declared before, `buf` takes 0x80 and `ap` lands above it (sce_com SceDebugDisp).
- Constant-pool order across a function with `if (mode == 0) {loop} else {totals}`: the target lists
  the else-branch totals BEFORE the loop's per-frame steps and the steps are `total / 30.0f`
  folded (one ulp off the decimal literals) — `const f32` locals at the top in the target's pool
  order (totals with the target's signs, then `ryA / 30.0f` steps, then `10.0f`), every body use
  written as the folded literal/expression (sce_com OpenBoxMain; SceElevator's 100/10/2 pool order
  = the assignment order `maxSpd; minSpd; accel`).
- `if (len != strcspn(..)) return 0; if (strncmp(..) == 0) {...; return 1;} return 0;` gives the
  target's `li r3,0` before the first `bne end` with the second test jumping to the out-of-line
  `li r3,0` (debug symbol_check; the nested-if form shares one `li` at the end).
- debug processBarDisp: the per-task rows print `TICK_1000F` (x1000, pool 1000.0 after 28.0), the
  spinner table is `static u8 xchr[5]` (no `extsb` on the `%c` argument).
- t_option: `Draw_pos` is C (`extern "C"`; a C++ declaration references `Draw_pos__FP3Veci`), the unit
  ends with `asm(".section .sdata,\"aw\"\n\t.balign 8\n\t.text")` (4-byte .sdata pad).
- Masked-compare tool of record for this pass: /tmp-style script over `tools/elffile.py` — relocated
  fields masked on both sides, relocation targets compared as (section, offset) for defined symbols
  and by name for undefined ones, same-section branches resolved to (containing symbol, offset),
  `_GLOBAL_.I/.D.<key>` canonicalised to `global_constructors/destructors_keyed_to_<key>`, functions
  of unequal size aligned with difflib. objdiff's percentages hid 4 byte-identical functions per unit
  on average (dtk `Sym+off` relocs).
- OPEN after this pass (register allocation / tie-breaks, ~1-15 words each, all forms in the
  per-function notes tried): card makeCardStatus (local-alloc order of the 0x40/0 constants and the
  fmt2/spd2 loop temps), card saveMain (three `stb step; b` tails cross-jumped into case 1's copy:
  needs a CODE_LABEL before the scanned `stb`, jump.c minimum 2 for jump-chain candidates), option
  retry_load_menu/controller_menu/brightness_menu (global-alloc priority `o` vs `old`; the clamp's
  then-arm stores `stb r9` in both arms in the target and cross-jumps the store), t_option tp_pl_flag
  (`li r4,0xfe` before `addi r3,r30,ItemMgr@l` in both arms: the high pseudo dies at the `addi` in
  ours, weight -1 wins; the target orders it last — dying-register tie-break not applied, same family
  as emrock SetRock), merchant sellPrice (the two pool `lis` r11/r10 vs r9/r11 around a psq_l), sce_at
  sceAtFunc_pos_jump (f0/f13 of `dstAngle` vs `0.0f`: qty priority by post-sched1 range),
  SceAtCheckSystemItemSet (COMPILER-DIFF 6: the target keeps case 0x1001's `bl RandomItemCk` tail
  and jumps case 0x1002 back into it), cam_qfps init (11 reference stores issued in pure source order
  by the target — the dying-store rule not applied), title stageSelect (`li r30,0` right before
  MercSysGetSaveWork without a store-flag), titleSub (2-iteration `mtctr` loop), puzzle orientation
  (`o - 4` in CTR: `subic. r0,r29,4; mtctr r0; mfctr r31`), puzzle size_y (RETURN cross-jump of the
  last arm skipped when it is followed by `li r3,0`), cam_ctrl switchCamera (loop counter/pointer
  r10/r11), route_ck RouteCkPosToPos (`lbzx` index in r0: the tbl base is pointer-flagged in the
  target), debug ProcessTickGet (proc_name store before proc_tick in the else arm), main_mem
  MemCheckHeapEnd (the `d->allocated` reload after the if/else; `li r3,0` between compare and branch).

### Small tool RELs, fourth pass (t_light/t_scroll 37/42, t_movie/snd_test 58/77, t_movie/t_snd_vol 13/27 written; 2026-09)

- bcmp-style byte compares align functions by symbol name: an out-of-line header inline (varargs
  `cLog`-style body, no symbol in our object, `fn_<mod>_<addr>` in the split) shows as MISSING even when
  its bytes match — compare the `.text` tails by hand before counting it as a residue (t_scroll).
- Original-only shapes found in snd_test (none reproducible, all left as residues):
  - A loop-invariant `Snd_voice_work` address that our reload rematerialises with `lis/addi` is, in
    the original, spilled to a constant-pool word (`lis/addi/lwz 0(r)` of an anonymous `.rodata` word
    emitted after the function's last string, i.e. at pool position) and costs one more callee-saved
    register (disp_sequencer: one more stack slot for `ch_prio`). `static T* const p = &g` folds,
    `*(T* const*)&p` gives a *named* word at declaration position — neither is the pool shape.
  - Identical 1-char string literals (`">"`, `"<"`, `"D"`) are NOT merged across function groups in
    the original (3 copies of `">"`, 2 of `"<"`/`"D"` in one unit) while longer strings are. Ours merges
    them; distinct constants with identical bytes come from `">\0"` / `">\0\0"` literals
    (`// COMPILER-DIFF: candidate #10`). Within a function (or a group of functions that the original
    seems to share one literal between) keep the plain literal.
  - `w->cur += dir` on an s8/s16 member with an int `dir` gives `add r0,rDir,rCur` in the original; every
    source form (`dir + w->cur`, narrow locals, int temps, inline helpers) gives `add r0,rCur,rDir` here
    (test_tbl_now_check, test_req_no_select: 1 word each). Shortened narrow arithmetic swaps a
    `(subreg dir)` operand behind the loaded pseudo in our expand_binop.
- `switch (w->mode) { case 5: ..; case 6: case 7: ..; default: .. }` is the source of the target's
  `cmpwi 5; bne; ...; blt default; cmpwi 7; bgt default` chain (Snd_test_mode), not an if/else-if.
- A function whose every `return 0;` shows no `li r3,0` in the target is `void` (snd_test
  test_mode_menu/test_mode_move: the callers ignore the result).
- Calls that pass a stale `r3` (no `mr r3,r31` before the 2nd/3rd `bl`) come from a hard-register
  argument variable: `register SndTestWork* p asm("r3") = w;` then `f(p)` per call — GCC 2.95 never
  restores an explicit hard-register variable after a call (Snd_test_disp_basic).
- `lis/addi` + `stw r0,0xb0(r9)` for a single store to a global struct field (instead of the folded
  `stw r0,sym+0xb0@l(r9)`) means the field is `volatile` (SND_CTRL_WORK::dma_busy; the DMA wait loop
  `do {} while (ctrl->dma_busy != 0)` re-reads it for the same reason). Pointer-base form via a local
  `SND_CTRL_WORK* ctrl = &Snd_ctrl_work;` alone is folded back by cse.
- Zero-store order `stw 0x98; stw 0x90; stw 0x94` interleaved with an address computation is decided
  by source order through the scheduler: the target order was `dirNum(0x90), dirTop(0x94), dirCur(0x98)`
  in the source (directory_open) — try the permutations in a harness rather than reasoning.
- String helpers: `while (*s++ != '.') {}` is rotated with a duplicated head; `do { c = *s++; } while
  (c != '.');` gives the target's `bne <function start>` loop. `char c = *s; while (c) { ..; c = *++s; }`
  gives `lbz; extsb; cmpwi; beqlr` + `lbzu` (change_to_cap); `while ((c = *s++) != 0)` gives the
  `b test; body; test: addi; extsb; cmpwi` shape (get_dir_level).
- A `path[i + 1]` whose target `add` has the pointer first (`add r9,r3,r11`) is `*(path + i + 1)`; the
  plain subscript puts the index first in the sum here (dir_name_up).
- `if (a) return 0; ... return 1;` vs `if (!a) { ...; return 1; } return 0;` decide whether `li r3,0` is
  inline before the branch or out of line (blk_no_check inline, test_move_epara_* out of line) — check
  each leaf function's tail separately.
- A `s8 no = w->auxCur;` local set in every switch arm gives `extsb` at the load; the target's `lbz r11`
  in every arm (also in the default arm that has no use) plus `extsb` at each use is gcse PRE of a
  re-read `w->auxCur` used after the switch: drop the local and re-read the member.
- t_snd_vol: `GXColor`-looking colour parameters that arrive in a GPR (`stw r4,slot; addi r4,r1,slot`)
  are `u32` in the original (V4 passes aggregates by reference: our `GXColor col` param gives
  `lwz r0,0(r4)`); pass `u32` and cast `(GXColor*) &col` at the Tprim call. The tool's work pointer is a
  `Debug_alloc`'d struct reached through a `static` pointer that the original reloads after every
  store: same struct-member wrapper as t_scroll (`struct SndVolWorkPtr { SndVolWork* p; }`), initialised
  `= {NULL}` so it lands in `.data` next to the tables.

### REL near-miss sweep (cSceObj x3, Tools/t_util Matching; r119 26/27, em3a 40/42, t_camera_data .rodata; 2026-09)

- A constant-pool `lfs` the original issues AFTER struct stores through `this` (cSceObj setMove1_all's
  0.01 after the two Vec copies): pool loads are `RTX_UNCHANGING_P` and never depend on stores, so the
  constant is a function-local `static const f32 rate = 0.01f;` (emitted at its declaration = exactly
  where the function's pool would be) read ONCE through a `const f32&` inline (`FCRef`) into a local:
  the reference read is a MEM with neither the struct nor the scalar flag and `true_dependence` keeps
  it below the stores; a direct use folds to the literal, two reference reads reload between the stores.
- Stores whose following `pG` load must stay below them (r119 `sat[i] = SatMgr.create(..)`): a typed
  `PSetSat(cSat*&, cSat*)` reference store with the call as the argument expression (the work pointer
  is still reloaded after the call, r3 stored directly). The pG load position moves the two `addi`
  table-address uses next to each other, which is what breaks the global-alloc live-length tie of the
  pos/rot `lis` pseudos (pairs 1/2 were tied at 146/162 insns and fell to pseudo order).
- Register-order lever: a loop counter and a strength-reduced giv with the same refs/lifetime are
  allocated in pseudo order (counter first -> higher register); one more reference to the counter
  (`if (best == -1) best = j; else if (..) best = j;` — two statements that jump2 cross-jumps back into
  one `mr`) makes it 5 refs vs 4 and wins the register (t_util TutilGet3DPosXZ j = r10, j*4 = r8).
- A `pG` read whose `lis pG@ha` the target issues after a preceding member store (em3aPatrolInit
  `w->pRoute = 0; if (pG->pRoomEmi == 0)`): read `pG` through a reference inline (`GRef(pG)->x`); the
  load then depends on the store and the store outranks the `lis` in sched1.
- `.rodata` string order with two literals in one function in reverse code order (t_camera_data
  "B404"/"EMPT"): `const char* const tag = "B404";` at the declarations parses the string first and
  folds to the literal at the use (a non-const `const char*` local is kept in a register).
- Sched1 register-weight rule, loads: `INSN_REG_WEIGHT` is +1 per SET (stores included) minus 1 per
  death, so in a 12-byte template copy the word-8 load (last use of the `addi` base) has weight 0 and
  is issued before the word-4 load (+1); sched2 (post-reload, no weights) only re-sorts through the
  hard-register anti-dependence chains. The original issues 0,4,8 in r22a RopeMove / r40f BombSet
  copy 1 and 0,8,4 in BombSet copy 2, so its tie-break is not this rule (OPEN, no source lever).
- Local-alloc FPR order of constant temps (r40a first_init, r119 third SetTree block): qty priority
  is refs*size/length with ties by qty number (assigned scanning the block BACKWARDS, so the later-
  dying temp wins ties); f0 goes to the first allocated. Our sched1 issues the loads in LUID order
  (x, y, addi, z), the original's allocation says z's range was not the shortest. `asm("" : "+f"(px))`
  (two deaths -> global alloc -> f0) reproduces r40a's bytes but is not a documented COMPILER-DIFF.
- cse1 path structure (r120 R120Event): our cse falls through into both `if (!(flags & 0x10))` event
  bodies (their `high(EvtMgr)`/string highs become r29/r30) and the tail after the outer `if` starts a
  new ebb whose PRE copy is rematerialised (`lis r9, pG@ha`); the original skipped both bodies
  (fresh `lis` in each) and carried `high(pG)` into the tail (r31). Nested and two-`if` forms compile
  identically (jump threading). OPEN.
- t_mv mvInit: the then arm's `cursor = 0` literal always finds the SImode `zero` pseudo (or the
  known-zero loaded byte) through cse `src_related`; the `asm("" : "+r"(c))` launder becomes
  `mr r10, r29` (cse substitutes r29 into the asm input). Not a launder case.
- Tool: `mcmp.py`-style masked compare (functions aligned by normalised name, reloc fields masked on
  both sides, same-section branches resolved by target) plus `perm.py`/`variants.py` (statement
  permutations / listed variants between two marker comments, ~0.35 s per variant) is the loop that
  found the r119/t_util/em3a/t_camera_data fixes; keep them in a private /tmp dir.

### cam_extra "regression" audit, pendulum, db_cam (cam_extra 32/43 + vtable order/.sdata fixed; pendulum 7/12; db_cam 7->9/13; 2026-09)

- cam_extra did NOT regress: compiled against `include/` of every commit since 13:50 (010a850, 84b57ea,
  b9503f2 cModel refactor, 7ea5eb6 t_camera.h, 4b4f215) with a `-I` override, the object is identical
  except for the DECL_UID suffixes of its static locals (32/43 byte-identical by masked compare, 31 at
  objdiff 100%; the `18/43` figure is not reproducible by either measure). The `.rodata -8` (pool
  order/content of the 11 unmatched functions: 300000/0.5/0 order in AttachedToMotion::move, a
  `bf9c61ab` vs `bf9c61aa` 70-degree constant, two missing 0.5/100000 words in FocusAnimation::move,
  a missing 0.05f, an extra 1.0f) and the `.sdata -4` (8-byte alignment pad before cam_qfps, now
  `asm(".section .sdata; .balign 8")`) date from the unit's first version. What was wrong since 13:50
  and is fixed: the vtable emission order. cam_extra.h declared the classes as cCamera, IDApplication,
  IdScope, IdBinocular, LookAt, PushObject, LookDownEm, Scope, Binocular, AttachedToMotion; the DOL's
  reverse-declaration order (LookDownEm, LookAt, PushObject, Binocular, IdBinocular, Scope, IdScope,
  AttachedToMotion, IDApplication, cCamera) means the header must declare them in the unit's `.text`
  order with each Id class right before its camera (done; cam_ctrl/cam_motion/r10b unchanged).
- Measurement note: a masked compare must strip the `.NNN` DECL_UID suffix of static-local symbols
  (`ZOOM_LIMIT_0.1301` vs `.401`) and must not compare the *content* of relocated words inside
  `const char*` tables (db_cam menuHitDisp's menu table: 0 in the split object, the string offset in
  ours) — both are false diffs.
- pendulum penClothAtMake (36 -> 16 words): `wk = (PenAtWork*) 0xE0000000` assigned AFTER the early
  return (a declaration initialiser hoists the `lis` above the test); `wk->num = n; wk->pAt = a;` in
  that order (both non-dying, RTL order); `pGS->flags_60` (struct view) for the debug-draw test in
  case 0 — with the plain `pG` the fixed-scalar `lwz pG` is hoisted between the `a->p0 = c` word
  stores and, since `lwz r9, pG` re-sets the copy's r9 temp, the `stw r9, 4` store gains an
  anti-dependant and outranks `stw r0, 0` in sched2's depend_count tie-break (copy issued 4, 0, 8).
  OPEN there: (a) `PSMTXMultVec(p0->mat, &at->p0, &v0)` issues `addi r5, r1, 8` first in ours, last in
  the target: after reload the frame register r1 is call-used/fixed, so every later CALL_INSN in the
  block anti-depends on every earlier r1 USE (haifa never clears `reg_last_uses` for call-used regs),
  and the second frame-address arg set (`addi r5, r1, 0x18`) gives the first one an extra dependant
  (4 vs 3) — the same mechanism as the r9 case, but here no source form removes it. (b) &v0/&up/&ax
  callee-saved order (target r25/r23/r24, ours r23/r24/r25): &v0 is the frame-offset-0 local whose
  address pseudo comes from `(set reg (reg vsv))`, which instantiate_virtual_regs turns into
  `(plus fp 8)` WITH a REG_EQUAL note; update_equiv_regs then doubles its REG_LIVE_LENGTH (458 vs
  227 for the other two, whose `(plus fp N)` sets carry no note), so 3*9/458 loses to 2*7/227. The
  target's order needs the &v0 pseudo to win (>= 5 weighted uses or no doubling); a `Vec* pv0 = &v0`
  at the loop top gives the 5th use but lets cse (AROUND path over the switch dispatch) rewrite case
  0's `addi r4, r1, 8` into the pseudo, and inside case 1 it is a user variable used in another block
  so loop.c refuses to hoist it. Not solved.
- penClothAtCkParallel / PenClothMove/2/3 (68/456/245/274 words): FPR/GPR allocation of the
  sphere-case temporaries (`rr = a->r * a->r` scheduled before the v.y/v.z chain in the target),
  callee-saved assignment (`w` = r31 above `parts` in the target, ours the reverse), a second hoisted
  copy of the 0x8c pool constant in Move's loop preheader (`lfs f30` + in-loop `lfs f29`), and the
  `addi r5, r1, 0x38; mr r28, r5` arg-first/copy-after shape. Not attempted beyond inspection.
- db_cam drawGround (now byte-identical): `const f32 unit = 1000.0f;` must be declared BEFORE `int n
  = big ? 60 : 30;` — the const's pool `lis` is then expanded in the block before the ternary's
  diamond while its only `lfs` is after it, so REG_BASIC_BLOCK < 0 and update_equiv_regs moves the
  single-use `lis` next to the `lfs` (late `lis r9` in the target, `neg r8/lis r7 0x4330` for the
  other block temporaries); declared after `n` the `lis` is block-local and stays at the block top.
  The zero chains are `a.z = b.z = a.y = b.y = 0.0f` (issued b.y, a.y, b.z, a.z: RTL order, the zero
  register lives on) and `a.x = b.x = a.y = b.y = 0.0f; a.z = neg * 1.1f; b.z = pos * 1.1f;` (the
  chain BEFORE the products: a.x is the zero's last use and is issued first, then the dying product
  stores, then b.y, a.y, b.x).
- db_cam moveOnPlaneXZ (now byte-identical): the column reads are cam_sys.cpp's `getColumn(m, c,
  &v)` inline for all three vectors; only `vx` (frame offset 0) shows the pointer form (`addi r31,
  r1, 8` kept callee-saved, `stfs 4(r31)/8(r31)`, `.x` via the frame, `mr r3/r4, r31` for the later
  PSVECScale) because integrate substitutes the constant `(plus fp N)` address for vy/vz.
- db_cam menu (82 -> 79, .text +4 short): `FSet(pG->Cam.param.roll, 0.0f)` reloads `pG` for the
  following CameraSetOrientationUp. OPEN: the target masks `cam_mode` once (`lbz r0; clrlwi r11, r0,
  24`) and uses r11 for both `old_cam_mode != cam_mode` and the `switch`; the COMPILER-DIFF 2 launder
  (`int cm = cam_mode; asm("" : "+r"(cm)); (u8) cm` twice) reproduces the shared mask but reuses r0
  (the raw byte dies in the target it does not) and flips the this/joy allocation (r31/r30). The
  `campos` word-0 temp in r30 (callee-saved, ours r3) is open too. menuFlag (141), move (160, ours 8
  bytes longer), adjust_qFPS (172) not attempted.
- Work-pointer `lis` above the calloc `bl`: store through a pointer to the member
  (`R205Work** wp = &work.p; *wp = MEM_CALLOC(..)`), placed after any earlier `pG->` op in the block.
- `SetPosXYZ(m, x, y, z)` static inline owning the `Vec` local and the call: shared temp slot, args
  evaluated before the stores, `addi r4,r1,8` per call (no PRE'd pseudo).
- `AtariFlagsAnd(&p->atari, mask)` keeps `addi r9,p,0x2B4` + `lhz/sth 0x1A(r9)`; the u16& form folds.
- `BitCk(f, A) || BitCk(f, B)` keeps two `andis.` on one load; `(f & A) || (f & B)` merges the masks.
- `switch (cut) { case 0xD: case 0xE: ... }` gives `cmpwi 0xE; bgt; cmpwi 0xD; blt`; the `&&` range
  folds to `addi/cmplwi`.
- `while (A) { if (!B) { S; } else break; }` keeps B at the body top; `goto found;` instead of
  `break` keeps an un-rotated poll loop.
- A pointer local redefined later (`m = NULL; ... = m; ... m = f()`) keeps its `li` at the entry;
  a single-use zero has its `li` moved to the store.
- `on = (prev ^ cur) & cur` gives the target's `addi` order; `hp = 1` in the `mode == 1` arm
  stores mode's register (cse jump-equiv).


### Small tool RELs, fifth pass (t_esp/db_port 51/68 written; t_camera/t_camera 19/57 partial; 2026-09)

- db_port.cpp (src/t_esp/db_port.cpp, D:/Bio4/Prog/db_port.cpp; .data/.bss byte-equal, .rodata 0x10
  short): the game bridge of the effect tool. Include set from the header strings: db_widget.h
  (atari/light/map_obj/widget) then event.h. Everything except DB_DrawBox/DB_DrawBoxFill/DB_DrawString is
  `extern "C"`; `font_draw(u8*, f32 r,g,b,a, s16 y, s16 x, s16 z, s16 w, s16 h)` is this unit's own copy
  (not eprintf.cpp's) and `fontTMtx` is imported by name although static in the DOL (make_rel resolves
  DOL names from symbols.txt regardless of scope, no DOL edit needed). `SetToolLight__Fi` is global in
  the t_esp REL (LightToolStart calls it): tools/db_light_esp.cpp exports it like the Tools wrapper, the
  t_esp symbols.txt scope was flipped by hand. `DB_MODEL_FILES` (db_mod.h) is a 0x85C class with an
  inline ctor calling init(): EspToolInit's three `static DB_MODEL_FILES` locals produce the
  `if (!guard) { init(); guard = 1; }` pairs in .bss, followed by the nine `static u8 tbl[0x20]`
  texture-blend tables (function statics in text order), then the file-scope statics. The two
  `DB_MODEL_FILES::append` overloads (char* 0xE0 / void* 0x84) were named by hand in symbols.txt.
- Idioms found: `(f32) -(int) joy->trigL` = `lbz; neg; xoris` classic conversion, `(f32)(int)(u8) x`
  = `lbz; xoris` (DB_GetKeybordData); `int idx = Sp_char_ck(c) - 0x20; (idx & 0x1F) * 8;
  ((idx >> 5) & 7) * 16` = `clrlslwi 27,3` / `rlwinm 31,25,27` (font_draw); a `char name[5]` local is
  BLKmode and stored frame-relative while `char name[8]` (DImode) gets an `addi` address pseudo
  (LoadModEff); a `u8 zero = 0` declared mid-block AFTER the early-return compare gives the shared
  callee-saved zero of two later byte stores without hoisting its `li` to the prologue; `if (mode ==
  0) f = 0; else f = (u16)(8 << (mode - 1))` (SeqSet); `EstSet(m, -1, 0, 0, head, f | 1, 0, (u32) m,
  0xCF, 0)` is the C++ overload; `DB_GetCursorPos`: `if (parts == 0xFF || flag)` (not `flag == 0`);
  `if (ret == 0) { fail; return 0; } ok; return ret;` for the `beq fail` layout (LoadData); explicit
  `->pNext->pNext...` chains (a loop compiles to `mtctr`); `EspGetPathAddr(pe->id, pe->owner)` with the
  cEsp pulled into a plain local (`PullEsp(&esp, 6); pe = (DbPathEsp*) esp;` — the address-taken `esp`
  would be reloaded after every call); the sp_3dgrid sizes are `(f32) -anm->x4` / `(f32) anm->x6`
  (s16 fields) with `(f32)(int) anm->x0 * 0.5f` fallbacks and gen->x88/x8C read into locals right
  after DB_GetCursorPos; GXColor byte stores in a 4-byte ADDRESSOF local come out in pure source
  order (drawTexture2: g, a, b, r) and `Mtx m; Mtx44 proj;` declared after the colour calls give the
  temp-0x8/m-0x10/proj-0x40/col-0x80 frame.
- Residues (all register/schedule or compiler-build): GetActiveModel/DB_isGetComeEventTool/SeqSet keep
  `int on = 1; if (!(flags & bit)) on = 0;` (evtToolOn inline) but SeqSet's zero-argument register and
  the `r9/r11` naming differ; EspToolExit's `&CamDbg` stays `lis/addi` + `stb 0xf(r10)` in the target
  (ours folds to CamDbg+0xF; the target's shared `li r8,0` also feeds the GXColor word); DB_GetCursorPos
  moves `head` to r0 and loads `parts` into r3; DB_VecNullPartsPos/DB_VecMulEmPartsMat/sp_* differ in
  callee-saved assignment of the pointer params; EspToolInit (0x1FC8) is written but -0xCC: the target
  spills `&tbl1` into an anonymous `.rodata` word (the snd_test pool-spill shape, .rodata 0x28A8),
  hoists the `i + 1` increment into a stack slot and keeps `EvtDebug` in r28/r29 pairs. DB_ConfigLoad
  (-4) parses `[KEY] value` blocks into a 16 x 0x68 table (name, motNo, pos, ang, parOn/No/PtNo,
  parPos/parAng) — structure identical, register order open.
- t_camera.cpp (src/t_camera/t_camera.cpp) is PARTIAL: ToolCamera, tcDataInitialize, tcInit, tcMenu,
  tcSubMenu, tcEdit, the twelve tc*dat helpers, tcNext*Ptr, head/tail/next_suffix, tcQuit,
  tcToolCameraMove, tcPreviewOnOff, tcCurrentCameraNo are written (19 identical); tcEdit_select,
  tcEdit_area, tcArea*, tcDrawArea, tcEdit_camera*, fix_camera_dat, edit_rail_*, edit_frame_no,
  tcMoveOffsetPoint, tcCamera*Point, tcDrawOffset, tcDrawRail, tcLoad, tcSave are not. The unit defines
  the .bss pools (tcTypeTbl/tcAdat/tcCdat/tcLdat/tcWork) and .data (`tcTypeName[9]`, `tcOnOff[2]`,
  `tcMenuPos[12]`, pTc, the routine table, the main menu names). TcWork got the field names of the
  offsets this unit touches (include/t_camera.h); `TcAdat::poly` (TcPoly view at 0x58) is what
  tcAdatInit writes the corners through (`stfs 0x4(r27)` off `&a->num`). `tcNextCdatPtr(s8, int)`,
  `tcNextAdatPtr(s8, int, int)`, `head/tail_suffix(s8)`, `next_suffix(s8, int)` take s8 (no `extsb`
  at entry: `__FSc` mangling, symbols renamed by hand after the first sync). `CameraTargetDistance`
  (cam_sys.cpp) is imported by the REL: made non-static, declared in camera.h, DOL scope flipped.
- PITFALL (cost a REL mismatch): a partially written unit that declares the routine-table entries
  it does not define as plain (non-static) functions makes `sync_rel_symbols` flip their scope to
  global in the module symbols.txt; the split object's ADDR32 field then holds A instead of S+A and
  the REL's .data differs (t_camera .data+0x56C/0x570 = tcLoad/tcSave). Keep the scopes local by hand
  until the functions exist.

- Tools debug-tool editor template (include/dbg_tool.h, 2026-09): `cDbgFileSelectWindow` /
  `cDbgOkCancelWindow` (cDbgWindow-derived, 0x348 / 0x238), `cDbgButtonTemplate<T>` (cDbgButtonBase +
  `int (*pFunc)` and `void (*pUpdate)` `(int no, T*, cDbgButtonTemplate<T>*)` callbacks at 0x1C/0x20, 0x24), `cDbgEditWindow<T>`
  (cDbgWindowBase + pWork/numWork/rows/top/execMode/copyCursor/copyWinMode/copyBuf T/bufValid/
  pButton[128]/num/pCur/pTop/pBottom/the five work callbacks; virtual order dtor, GetCx, GetCy,
  SetCurrentBottomButton, ButtonAllUpdate, LocalUpdate, LocalDisp, SetCurrentTopButton,
  ButtonPushCheck), `cDbgToolMain<T>` (0x50: pMenu, pEdit, pSave, pLoad, pSaveOk, pLoadOk, pExitOk,
  mode, ..., the callback copies, vptr at 0x4C). Units: t_esp_area (36/38, ToolEspArea -36),
  t_lightarea (36/38, ToolLightAreaMain -36); .rodata/.data/.bss identical, function order identical,
  not flipped. Idioms found there:
  - A module's later copies of the header's linkonce functions are nameless (`fn_Tools_*`); pair them
    by the gap after the previous named function, and check the ORDER of the block: the FileSelect
    destructor is `virtual ~cDbgFileSelectWindow();` + `inline ... {}` defined AFTER LocalUpdate
    (deferred inline, emitted after Init/LocalUpdate), the OkCancel destructor stays implicit (synthesized
    when its vtable is written, emitted BEFORE its LocalUpdate).
  - The sizeof(T) gap under the tool's spill slots (ESP 0x98, LIGHT 0xD8) is the frame of a once-inlined
    helper with a T local (`T unused;` in CreateEditWindow): the inlined callee's frame is one stack temp,
    freed after the statement, and the loop body's `Vec pos, scr` (declared inside the `for (;;)`) reuse
    its start. A T local in LoadData would give two temps (LoadData is inlined again inside Update).
  - `for (i...; i++, w++) { AreaData* a = &w->area; if (IsWorkAlive(w)) {... w->flags ... w->areaNo}`:
    with `a` an unconditional giv, combine_givs makes the field loads a-relative (`lwz 0x30(rA)`,
    `lbz -2(rA)`) while `w` stays the stepped base; `a` inside the `if` keeps them w-relative.
  - Switch cases past cse's jump-following path length (PATHLENGTH 10 conditional jumps) lose the
    cse'd `&Joy`: the four-case AreaNoExec takes `JOY* joy = &Joy[0]` right before `rep = joy->rep`
    (so it does not live across the eprintf calls) and uses it in the cases; the return still reads
    `Joy[0].trg` directly (the pointer is dead at the join).
  - Consecutive `pG->flags |= a; pG->flags |= b;` fold into one `oris`; the target reloads pG between
    them: use BitOn/BitOff (global.h) for every flag update in the tool bodies, not only in Init/Exit.
  - The tool's first zero-initialised local is the zero register of the inlined cDbgToolMain
    constructor's 22 stores and gets the callee-saved register (`u8 wait` in ESP, `int preview` in
    LIGHT, declared before `tool`); the later `= 0` locals (u8 cnt, int cam, u8 wait, int plNoHit) take
    their zero from the most recent zero pseudo and spill in declaration order.
  - `if (m == 4) ... else if (m == 2) {} else if (m == 3) {} else wait = 1;` keeps the three compares
    (`!= 2 && != 3` folds to subi/cmplwi); empty `case 4: case 5: break;` in Disp shape its tree.
  - The window pointer of KeyCheck+LocalUpdate is a parameter (`WinUpdate(cDbgWindowBase*)`), the
    AddButton column loop reads `pEdit` once into a local, InitAllWork keeps `e = pEdit; w = e->pWork`
    locals: otherwise every store through the window reloads the tool member.
  - t_lightarea defines the module's `__builtin_new/__builtin_vec_new/__builtin_delete`
    (`Debug_alloc(size, 1)` / `Debug_free`) at the top of the unit; its works and the file image are
    Debug_alloc'd and `MakeSaveData` (SaveData without the file) feeds `LightAreaDataLoad` every frame;
    `((cUnitEventView*) pPL)->endEvent(0)` (st_mgr_event.h) is the `lha 0x18` virtual on the player.
  - OPEN: the edit window's `new` null test is compared BEFORE the constructor's AddButton loop
    (`cmpwi r31,0; mfcr r29` .. `mtcrf; bne` after `pEdit = e`; no branch around the body: not
    -fcheck-new), so the hoisted `preview == 0` compare takes cr7 (`mfcr; slwi 28`); the three
    OkCancel Init copies materialise `li r0,0` per window for pCur/pTop/pBottom while x1C/x20/num share
    the hoisted zero (one pseudo here, chains/NULL/order do not split it); cDbgWindow::Init (db_toolbase,
    19/20) and cDbgFileSelectWindow::Init schedule `li 1` before `li 0` and keep the LAST zero store
    last, ours issues `li 0` first and hoists the last zero store to the top (the FS path-pointer stores
    also move up); statement order, chains, in-class/out-of-class, an inlined base Init do not change it.
    ToolEspArea's r14/r15 (wait vs Joy@ha) and r22/r23 (`&tool`) swaps follow from those.
- cse and in-struct stores: the FIRST load of a global pointer decides whether a later in-struct store
  invalidates it; use the struct view (`pPLS`) for every read that must stay ordered, not just one.
- `static inline SetAngV(cModel* m, Vec* v) { m->setAng(v); }`: every call is a fresh `addi r4,r1,N`
  (integrate substitutes the caller's address into the hard-register arg set) -- no PRE copy.
- Peeled first iteration with a duplicated `y = lim` store = `step; if (y < lim) y = lim; else { wait:
  sleep; step; if (!(y < lim)) goto wait; y = lim; }`; `spd = 0.0f` after the preceding call.
- `(T*) ((i) * sizeof(T) + (u32) p + 0x10)` puts the array offset last and index-first.
- Two sibling `if` bodies needing the same Vec slots after a block that used one: declare the Vecs
  once mid-function (per-block declarations best-fit into the earlier block's slot).
- Three identical blocks with per-block cEmWrap member and task function: a macro, not an inline.
- `EM_LIST(0x19 + (u32) n % 3)` gives `mulhwu 0xaaaaaaab; srwi 1`; `(f32) n` with `u32 n` gives the
  2^52 magic without the 0x80000000 word.

- Vec temporaries loads-before-stores: `f32 x = a->pos.z; f32 y = ..; f32 z = ..; v.x = x; v.y = y;
  v.z = z;` (plain member assignments interleave load/store; `FSetP`/Vec* helpers get hoisted).
- `!(x & 1)` folds to `xori; andi.; beq`; the target's `andi.; bne` needs `(x & 1) == 0`.
- Un-rotated `for(;;)` with calls: keep every exit in an `else` arm (`if (c) { body } else break;`).
- `!(f & A) && !(f & B)` folds to one test; the target's two `andis./andi.` need nested ifs.
- `FAdd(SmdGetObjPtr(n)->pos.y, spd)` keeps a static-pointer load below the store.
- `u32 max = *(u16*) mot; (f32) max` gives `lhz` + `stw/lfd`; a `u16` local gives `sth/psq_l qr3`.
- `int i` over `work->em[i]` gives count-down `li 0x70; subic.`; `u32 i` keeps `cmplwi`.
- `p->total = p->cnt = call();` stores cnt then total from one work-pointer load.
- Second cEmWrap alias `cEmWrapSetPtrI(...) asm("setPtr__7cEmWrapsSci")` for an untruncated u32 (#4).


### Tools/t_sce t_sce_at.cpp (shared object, 58/59 named functions byte-identical, sections identical; 2026-09)

- src/tools/t_sce_at.cpp (D:/Bio4/Prog/t_sce_at.cpp, mem_alloc line 0xBE3): the scenario-area ("AEV") editor,
  the same object in Tools and t_sce (`bcmp` 58/60 in both; the 60th is the nameless cManager<cLight> block).
  Include order from the header strings: light.h, atari.h, dmg.h, map_obj.h, widget.h (explicit includes). Every
  function but `ToolSceAt()` is `extern "C"` (statics inside the block). Work = `Debug_alloc(0x19DB8)` behind the
  one-member struct `sceAtWk.p`, the edited area behind `sceAtCur.p` (.bss 8 = the two pointers): header 0xFC,
  `SceAtWork area[128]` 0x10C, load/save image 0x4F0C, copy buffer 0x9D1C, then two 512 x 64 message-name tables
  (loadMesName parses `#define MES_` of the room / common mes headers). `SceAtSys` (static in game/sce_at.cpp) is
  imported by name through a local `TSceAtSys` view; the DOL needs no edit (make_rel resolves DOL names by name).
  Not flipped: `tSceAtDataInput_basic_menu` is -0x24 (below). NOTE for the flip: t_sce has three `set_filename`
  (t_block, t_sce_at, t_sce_item, all `global` in the .sym); the compiled copy defines a global `set_filename`
  next to t_block's split one — check the -r link (or make ours static and re-sync) when flipping.
- Idioms that mattered here:
  - u8 wrap-around `field = n < 0 ? hi : (n > hi ? 0 : n)` must go through a second variable (`m = ...; field = m;`)
    or the store is duplicated into both arms (`stb` per arm); the 0..hi clamp is the if-form
    `if (n >= 0) { m = n; if (m > hi) m = hi; } else m = 0;` with `m` a SEPARATE variable (`CLAMP(n, n, ..)` ties
    the copy away: no `mr r0, n`). With a variable upper bound put `int max = num - 1;` first in the then-arm
    (`subi` scheduled before the `mr`).
  - `y = pW->y; y += 0x10;` (two statements) keeps `lha` (a second set stops combine from narrowing the load to
    `lhz` through the `extsh`); `y = pW->y + 0x10` gives `lhz`.
  - Three editors (ladder_main, pos_jump_main, cam_ctrl_main) address the payload as `pCur->ladder.pos.x` etc.
    (pCur reloaded per access); the others (mes, flg, shd, damage, scr_at, use, save, field) use a payload pointer
    `T* d = (T*) pCur->data` (r31 = pCur + 0x5C, loads `lwz 0x5c(pCur)` folded, stores `stw 0(r27)`).
  - `if (d->flags & 2) eprintf(...); fl = d->flags; if (fl & 2) { draw }`: the second test through a `u8` variable
    stops jump.c's thread_jumps (a REG_USERVAR on the Y side fails rtx_equal_for_thread_p) — the target keeps both
    tests; the plain second `if (d->flags & 2)` is threaded into the first `beq` (-4 bytes, branch to the end).
  - `u8 em = pCur->x39 & 9; if (em != 0)` gives the target's `andi. r0,r0,9; cmpwi r0,0` pair.
  - Case-local pointer copies where the target keeps pCur in one register across skipped blocks: case 2
    `SceAtWork* a = pCur;` (the store after six `if` blocks uses the case-top register; the cse path budget
    PATHLENGTH 10 is exhausted from the function entry), case 4 `u8 f = a->x38` reused for the RIGHT switch's
    `a->x38 = f | 0x80` while the LEFT switch and the final store re-read `pCur->x38`.
  - door_PosSet: the `dstPos == 0` test reads the constant through `static const f32 zero = 0.0f` + `FCRef`
    (the pool word sits before the function's pool, its `lfs` stays below the pPL struct copies), the pPL copies
    are `memcpy((u8*) pW + 0x14, &pPL->pos, 12)` (pPL reloaded between), the pG next-room stores are
    FSet/U16Set/U8Set (pG reloaded after each), the return copy `FSet(pCur->dstPos.x, pPL->pos.x)` (pPL reloaded),
    `Vec d = {0,0,0}` declared inside the loop body (memset per iteration), and `pCur->dstPos.x == (z =
    FCRef(zero))` inside the condition for the r3/r4 order of the two `lis`.
  - Menu enable chain `create[1] = create[2] = edit[3] = edit[4] = valid` (store order create[1], edit[4], edit[3],
    create[2]; createMenu's address pseudo first) — the t_se_at EditMenu r7/r8 open item is the same thing.
  - DataLoad: `int ret = 0` block-local inside `if (sel >= 0)` (li r3 after the blt), a separate `int cmp` for the
    strcmp result, error arms written first (`if (version != 0x104) {err} else { cmp = strcmp(); if (cmp != 0)
    {err} else {ok} }`), literal zeros in cases 8/9. DataSave: a typed `TSceAtFile* p` for the memcpy (crclr libcall).
    tSceAtExit: `if (sel >= 0) switch (sel)`. loadMesName: `u32 n` assigned after the early return (`cmplwi 0x1ff`,
    `li r27,0` after the call), `dst = (char*) (n * 64 + (u32) names)` (index first in the add).
- OPEN (tSceAtDataInput_basic_menu, -0x24): gcse PRE hoists `high(sceAtCur)` into r27 for every case body
  (`lwz r11, @l(r27)`), the original keeps a fresh `lis r9` per case start and per post-clamp store and uses r27
  only in the display tail. In our .gcse dump expression 0 (high(sceAtCur)) is redundant in 13 blocks with the copy
  inserted at the end of bb 0; the target's case-start occurrences were not replaced (rematerialised single-use
  pseudos). Case-local `SceAtWork* a` copies fix individual cases but not the pattern; not understood.
- db_mod.cpp (Tools 0x9978 / t_esp 0xA0EC): NOT written. Facts gathered: t_esp's build has `DB_MODEL_FILES::append
  (char*)`/`append(void*)`, `dbModMotionSet` (0x1AC) instead of `dbModMotionSetSeq`+`dbModGetMotFilename`, no
  `dbModGetViewFlag`/`dbModUnsetViewFlag`, and the extra dbModBinName..dbModelSetAng0 loader functions before
  dbModelSetCamera (a define like db_light's). State `pDbModState` = Debug_alloc(0x3460) (view flag at 0x345C,
  0x3449 = load failed, routine index s8 at 0x10, 0xD8 set no, 0x1C..0x22 per-type numbers, 0x141 set name),
  `dbModSlot[64]` (0x2754 each: cEm* at 4, sixteen file pointers at 0x48/0x88, 0xD0-byte name slots from 0x108,
  0x148 seqFlag, mem_alloc(0x98, line 0xEB) at 0x1D4), a second 64-slot table at .bss 0x9D5E8, the mot_tbl.txt
  image (`Room/Em/mot_tbl.txt`, .data 0x500) parsed by mottbl* into a 0x4C.. table at .bss 0x13AAEC; .data = the
  15-entry routine table + `TOOL_MENU`-like `{name, id, flag}` 16-entry table + the string tables and the option
  values (0x46, 0x12C, 8, 0xB, 0x32, 8).
- Cross-jump survivor for a shared `return 0`: `switch` with `case 0: default: return 0;` first and the
  last case's inline `if (A && B) return 0;` with NO trailing return (em10ClimbOverCk).
- Nested switches returning 0/1: outer `default: return 0;` FIRST; inner arms `case ..: break;
  default: return 0; } return 1;` -> per-inner `li r3,0; bnelr; li r3,1; blr` (em10ArmorCk).
- Reload hands scratch registers out round-robin over the spill set: identical arm bodies with one
  such insn alternate r9/r11 and cannot all cross-jump (three arms -> 1 and 3 merge).
- cse's `find_best_addr` rewrites `(mem (reg w))` to `(mem (plus em 0x3e0))` only for the zero-offset
  member, so `w->flags` and `em->x3E0` merge while `w->x16c` stays w-relative.

- A `Mtx` declared at function scope sits below block-scoped Vecs and can make gcse PRE insert a
  frame `addi` at a loop entry, which flags the loop "phony" and blocks all invariant hoisting --
  declare the matrix in the block that uses it (r224 `reva_move` 71% -> 98.7%).
- `pPLS->rot.y = ..; pPLS->setPos(&p);` reloads `pPL` between the store and the call when a template
  copy lies between them.
- A virtual call then a member store on the same global object keeps the pointer callee-saved only
  through a local (`cPlayer* pl = pPL`).
- Check identity with a label-normalised instruction diff: objdiff marks `bc` REL14 relocs as REPLACE
  with identical bytes.


### Tools REL flags-first sweep (t_flr_at Matching; t_motseq 17/21, t_vib 24/29; 2026-09)

- t_flr_at (26 -> 29/29, flipped): the tool loop's `Debug_alloc` store uses the loop-hoisted
  `lis flrAtWk@ha` only through the reference form (`FlrAtWork*& wp = flrAtWk.p; wp = Debug_alloc(..)`,
  t_motseq idiom); the title lookup is index-first (`*(const char**) (editType * 4 + (u32) tbl)` ->
  `lwzx r8, rIdx, rTbl`). flrAtDataSave: the save counter is read and stored through references
  around the 0x84-byte record copy (`ISet(n, IGet(n) + 1)`, `IGet` = `int&` getter) so both the load and
  the store stay below the copy's `stw`s, and the u16 header count is `*(u16*) ((u8*) &n + 2)` — the
  target's `lis/lhz` carry `sym+2@ha/@l` (a plain `(u16) n` narrows to `sym@ha` + `sym+2@l`, a different
  REL relocation addend although the linked bytes agree). flrAtDataLoad: COMPILER-DIFF 2 launder as an
  inline `static inline u8 ColU8(int c) { asm volatile("" : "+r"(c)); return c; }` used *in the argument
  list* (a statement-level launder before the call moves the x/y argument loads after it); the `volatile`
  matters: the plain `asm("")` form let sched1 issue the following ternary's `lis/lwz` before the `clrlwi`.
- t_motseq msqFrameSizeCk: `max = (int) x << 6` as ONE expression gives the shift result a global.c
  hard-reg *preference* for the fpmem-load temp (set_preference looks through unary/binary ops), so `max`
  inherits the temp's local-alloc register (r9) and pushes the loop giv to r11; two statements
  (`max = (int) x; max <<= 6;`) make `max` a two-set pseudo allocated by priority after the giv.
- t_vib: (a) a `u32 c = col;` copy whose address is passed to a Tprim call is not a separate local when
  the target stores the incoming argument register into the frame (`stw r5, slot`): use `&col` directly
  (tvibListLineDraw); when the copy's store must stay ahead of the following pointer load
  (tvibFrameMarkDraw: `stw r5` before `lwz tvib`, and `size`'s copy `mr r5,r6` allocated after r5 is dead)
  the local is a `GXColor c` filled with `*(u32*) &c = col` (a cast-then-deref store, no struct/scalar
  flag, so the in-struct `V->scroll` load depends on it). (b) A record pointer used in two loops
  (`TvibData* d` at function scope, tvibFileSave) picks up the hard-reg preference of the second loop's
  memcpy argument copy and is `add r4, ..` in BOTH loops; block-scoped `d`s give r9 in the first loop.
  A header `memcpy(p, f, size)` must pass a *variable* size (a `sizeof` literal is inlined as a block
  move); `hsize = sizeof; memcpy(p, f, hsize); p += hsize; size = hsize;` puts `li size` after the call
  and `addi p` before it. `memcpy(p, d, len); p += len; size += len;` (size last, carrying len's death)
  issues `add size` before the call and `add p` after. (c) Colour-word stores: `col = C;` written AFTER
  the S16Vec corner stores is the dying store issued 5th/last (tvibMainFrameDisp), written first it is
  hoisted to the block top; the second poly block has its own `col = 0` after its stores. (d) `x = list_x
  + i * 230 - 2` allocates the product to r0; `(list_x - 2) + i * 230` gives the target's `lhz r0 / mulli
  r9` pair. (e) `top + i` written twice (index and eprintf argument) instead of an `int n` local gives
  the `add r0; mr r10, r0` PRE copy; the loop test is `i < 16 && top + i < 64`.
- OPEN, t_sce_at tSceAtDataInput_basic_menu (-0x24), mechanism narrowed: ours merges the case-body and
  x38-block `high(sceAtCur)` computations into bb0's pseudo in **cse1** (the path from bb0 skips both
  `on = 1` diamonds and follows the dispatch `beq`s), then gcse PRE inserts the reaching reg at the end
  of bb0 (also in the target: `lis r27` in the prologue) and the tail joins use it; the target kept fresh
  `lis` in bb2 and every case, i.e. its cse1 did not carry bb0's pseudo into them. Not a global flag: a
  scratch cc1plus with cse1 or cse2 running without follow-jumps/skip-blocks (and the -fno-cse-* flags)
  regresses 13-35 other functions of the unit and leaves basic_menu unchanged. t_vib tvib_R0_VibLoopSet
  is the mirror image: PRE inserts `high(tvib)` at the end of bb0 AND of arm 1, the skipped arm 1 re-sets
  the reaching reg, and ours rematerialises arm 2's copy (`lis`) where the target keeps the hoisted r10.
- OPEN, unchanged (register/schedule ties, all forms in the sources tried): db_toolbase cDbgWindow::Init
  and the FileSelect/OkCancel Inits (dying zero store not hoisted by the original, `li 1` before `li 0`),
  t_atari plmove10 (copy `stw`s after the RMW `stfs`s), t_mv mvInit, t_dr ListDisp/Menu_main,
  t_esp_area/t_lightarea mains, t_motseq QuitCk/Sequence/SeqResize/msqDisp, t_vib tvibModeFrameDisp (the
  target does not cross-jump the two 12-store arms: different temp registers per arm, `mode` in r7),
  tvibFrameLineDraw (frame +0x10, one more callee-saved), tvibListVibDraw, tvibEditFrameDisp; t_rck and
  db_mod not iterated (db_mod dbmodGetFilenames: the target strength-reduces `type * 0x800 + 0x3a1`
  as an outer-loop giv `li r22,0x3a1 .. addi r22,0x800`, ours recomputes `slwi; addi` per iteration).

### Player modules (pl11, pl06, pl02 Matching; pl0d/pl_wesker.cpp Matching; pl0a 38/39; include/pl_mod.h; 2026-09)

- Layout: the 8 player RELs are `PlInitFunc = PlXXInit` modules (`_prolog` also `OSReport`s "PlXX <NAME> prolog
  Ok"; pl11 sets `EmInitFunc` and has no OSReport), `PlXXInit(cEm*)` = `new (em) cPlXxx()`, the player class
  (cPlayer subclass: ctor, setMotion, move, setModel, setRightHand, setLeftHand, setFace, setHead x2, initCloth,
  moveCloth in that .text order), then `_prolog/_epilog/_unresolved`, the synthesized destructor and the
  in-class `initCloth/moveCloth` bodies when they are one-liners, then the cUnit linkonce copies. pl11 is a
  `cSubChar` subclass (`cSubAshley`: modelSet / setFace / setHand / initCloth / moveCloth; vtable slot 9 of
  cSubChar is `modelSet`, renamed from the pl_npc.h placeholder `endDamageCore`). pl0a and pl0d are
  multi-object: the weapon module's routine object rebuilt WITHOUT its `PlXxxMove` entry (pl0a: wep07's r2/r3
  routines + `wepDown` + the named cManager<cLight> pair, .data `wep07_func_tbl[5] = {ready, set, fire, 0, 0}`
  kept although unreferenced; pl0d: cObjRuger without `ObjRuger_init`, then the wep02 routines) followed by
  `pl_klauser.cpp` / `pl_wesker.cpp` (their `__FILE__` strings); modules.py UNITS pins the header-string
  groups. pl0e/pl0f (`pl0e.cpp`/`pl0f.cpp`, jet ski: cPl0e/cPl0f cEm subclasses with a 0x118-byte work at
  0x3E0, routine tables in .data, 0x34 COMMON) and pl14 (Luis: cSubLuis + cRoutine/cAction/cAnalysis/cVoice
  classes, cObjLuisItem, static-init pair) are single files, unwritten.
- The costume classes are templates of pl_leon.cpp / pl_ashley.cpp: `setMotion` is Leon's table verbatim
  (`PSet(pMotTbl[0x5F..], PL_ARC(0x32..))`), ctor = `init0; setModel; weaponRelease; weaponLoad(pG->wep_no,
  pG->wep_type); weaponInit; init1; setMotion; arc = pG->pPlArc; EspDataLoad(arc[0x1A], 3, 0); startUp;
  pFootShadowTbl = pl_fs_tbl` (Krauser moves EspDataLoad right after init0 and adds five stores + a pGS flag
  clear; Ada adds an `EstSet` under `costume == 1`), setRightHand/setLeftHand = Leon's with the module's
  archive indices and switch cases (`default: data = (void*) no;`; a `case 0..9` run with `case 1` in the
  middle gives the `cmplwi 1; blt / cmplwi 9; bgt` range tree), setFace = Leon's, setHead(int) deletes pHair
  (HUNK/Ada/Krauser: pShape) then creates ARC(0xB/0xC)+ARC(7). Error strings are copied between characters
  (Ada's setModel mixes "cPlAda", "cPlAshley" and "cPlLeon" messages; HUNK's is "cSubLuis::init() failed.").
- Idioms: (1) synthesized destructors (no user dtor) for the size-0x20/0x24 `_._` bodies; (2) `cModelInfo`
  members stored then followed by a pG read need `PSet` (pl02 pHair, Krauser pShape); (3) partner archive reads
  go through `subSelf` (`SUB_ARC`: `lwz 0x3e0; lwz 0x378`), `hp = pGS->sub_life` keeps the pG load below the
  vptr store; (4) `subHand[0] = create(); if (subHand[0]) addModel(subHand[0])` (member re-read, forwarded
  `mr r4,r3` after the compare) vs `info = create(); subHand[1] = info; if (info)` (`mr r4,r3` before the
  store) are the two hand blocks of cSubAshley::setHand; (5) a data table the REL addresses with field 0 is a
  global (pl02 `adaHair2P`, `adaHolsterAt`; pl_wesker `weskerJacketAt`): make it non-static or the ADDR16
  field carries S+A; (6) a module whose first object defines the cUnit copies: name its `_vt.5cUnit` label in
  symbols.txt/sym_map.tsv so a later unit's dtor binds to it (pl0d); (7) unreferenced first-object linkonce
  copies and later-object nameless blocks are handled by ngccc as documented, the nameless block has no symbol
  so a by-name compare reports it "missing" — compare the .text tail bytes directly; (8) the two `setHead`
  overloads are ambiguous for sync_rel_symbols: rename `setHead__NcPlXxxi` / `setHead__NcPlXxxPvT1` by hand;
  (9) cloth setters (`testHairSetAda2`, `testHolsterSetAda2`, `testJacketSetWesker`) follow pl_cloth.cpp's
  member order with the zero stores last and `x54 = 0` as the last zero; a `rate` local shared by `x40`/`x50`
  needs the order `pModel = 0; x40 = rate; x50 = rate; flags; x54 = 0` (brute-forced); (10) Krauser's
  transMove: `if (m->color[3] <= 0xBE) m->color[3] = a + 0x40; else m->color[3] = 0xFF;` (a store per arm —
  a single result variable lets jump.c hoist the constant arm above the compare) with `int a` for the signed
  `cmpwi`, the fade-out written out per arm with direct member reads (`u8 b` copy gives the `clrlwi`
  mask), and `krModel[2]` re-read in both arms of the pulse-length `if`; (11) `pl_R1_KlauserAttack`: the
  pool wants `const f32 hitLen = 3000.0f;` at the top (case-0x15 constant first), `BitOn/BitOff(pGS->flags_5018, ..)`
  after `x890` stores. OPEN: transMove's alpha-pulse block (30 words: t/a register names r0/r10 vs r6/r0 and
  the second half's constant-load interleave; do/while(0) placement, temp variables, declaration orders tried).
- Tools used (/tmp/plmod): `mcmp.py MOD[/UNIT]` (masked word compare per function incl. function order,
  .rodata/.data words + reloc targets), `perm.py MOD/UNIT SYM` (statement permutations between `// PERM_BEGIN`
  / `// PERM_END`), `variants.py` (`// VAR_ALT`-separated alternatives). Store-order blocks of 5-6
  statements are solved in < 1 minute this way.
- `pG->emlist` as an EmListData array view (`struct EmListView { u8 pad[0x52E8]; EmListData emlist[0x100]; }`)
  gives `slwi; lwz pG; add; lbz 0x52e8(r9)` with pG/sum tied to r9 (ARRAY_REF via get_inner_reference);
  the `EM_LIST(no)` pointer-sum forms rewrite MULT-first. em_wrap::setEm matched with it in all modules.
- `pGS->` for a pG read between a frame store of a call result and the next call's frame reload.
- sched2 anti-dependence priority: a `lwz rX,sym@l(r31)` placed by sched1 before frame stores gains +1
  priority (anti-dep cost clamped to 1) and moves above every store -- expect it whenever the
  `high(pG)` pseudo lands in r31 (r119); compiler-side.
- PRE copy before a call's parameter loads (`addi r3,r1,N; li r4,0; mr r31,r3`) = gcse insertion at a
  block that ends with the CALL_INSN itself (EH edge, no `(use 0)` nop) -- a scope object with a
  destructor reproduces it at the cost of a frame slot (r20f/r213 Init).
- A `#include`d shared source (em_wrap_v2.cpp -> em_wrap.cpp) is not rebuilt by ninja when the included
  file changes: touch it.


### Stage rooms, st2_2/st2_3 last-nine pass (r21d 31/33, r213 27/30, r223 28/29, r227 26/30, r214 15/25 written; sections identical; 2026-09)
- Units: src/st2/r21d.cpp, r213.cpp, r223.cpp, r227.cpp, r214.cpp (all with the `RxxxWorkPtr { p; }`
  struct-member work pointer). The `fn_<mod>_XXXX` 0x3B8 block mcmp reports MISSING is the cLight
  linkonce copy `place_linkonce_module` inserts (bytes present in the .o, nameless); ignore it. r212,
  r221, r226, r22c not started.
- `while (1)` vs `for (;;)` differ in GCC 2.95: a `for (;;) { ..; if (c) break; SceSleep(1); }` whose
  body starts with a conditional block gets rotated (`b body; L: SceSleep; body: ..; ble L`), `while
  (1)` keeps `..; bgt exit; SceSleep; b top` (r227 checkBox0Fall, setEmOnElv2 -- the latter also gained
  the target's `&local` copies `mr r23,r28` once written as `while (1)` with `continue`s).
- A flag variable cleared after a call (`SndStrReq(..); on = 0;` instead of before it) swaps which of
  two hoisted compares (`cmpwi cr7/cr6, on, 1/0`) gets cr7 (r223 StrCheck): the compare pseudo's
  post-sched live length decides.
- `if (RsfCheck(..) == 0) return 0; return 1;` gives `li r3,0; andis.; beq; li r3,1` (r223 isZouenGo2);
  `int v = 0; if (..) v = 1; return v;` keeps the value live across the call in r31.
- Store-order of `SetPos`/`SetAng` with literal coordinates where the object comes from a call: the
  target calls `SmdGetObjPtr` BEFORE the stores -- a block macro `{ cModel* m_ = (o); v.x = X; v.y = Y;
  v.z = Z; m_->setPos(&v); }` on the caller's Vec (r223 SET_POS_XYZ); an inline taking `Vec*` keeps
  the Vec address in a register instead (`addi r31,r1,N; mr r4,r31`).
- A loop test written `for (i = 0; (u32) i < 4; i++) { if (i == 3 || em[i].ckFindPL() == 1) break; ..}`
  keeps `cmplwi 3; cmpwi cr7 3; bgt; beq cr7` (r223 EmCheck); `(u32) i < 4 && i != 3` in the condition
  is range-folded to `cmplwi 2`.
- `u32 timer;` declared at the top but `timer = 0;` written right before the `for (;;)` puts the `li
  r30,0` after the init loop (r223 EmCheck); the r27 = timer+1 copy across the inner loop is automatic.
- Two `case` arms with identical bodies (`case 0: A; break; case 1: A; break; default: B;`) reproduce
  `cmpwi 0; beq A; cmpwi 1; bne B` when the bodies share one function-scope `void* mod` (identical
  stack slot -> cross-jumped); `case 0: case 1:` gives the range test `cmpwi 1; bgt; cmpwi 0; blt` (r227
  Evt_R227S02_Func). An extra `case 0: break;` in a funcMode switch adds the target's `ble end`.
- `R227Work*& wp = r227_work.p; wp = MEM_CALLOC(..)` keeps the following `lwz pG` below the work store
  (r223/r227 Init, the r21d idiom); `BitOn(obj->be_flag, 0x20)` before a `pG->room_id_prev` test keeps
  that pG load below the store (r227 initGondola).
- `cEmWrap e0; e1; ..` declared AFTER the leading `SceSleep`/`RsfSet`/`setEm` statements: the ctor calls
  follow them (C++ mid-block declarations, r227 setEm2/setEm3/setEm3_after/setEmOnElv1).
- Const-folded locals: `const f32 step = 10.0f; const f32 power = 20.0f;` declared where the pool wants
  them fold into their uses (fresh `lfs` per QuakeExec call, the step hoisted by loop.c) while fixing the
  pool order (r227 operateElv). A `static const Vec` INSIDE a function is output before that function's
  pool and copied at the use site (`v = gotoPos;`, r214 execCatapult); a file-scope `static const` table
  is output after the strings at the end (r214 r214_catTbl / r214_rockOfs).
- `int i` (signed) loop counters used only as array indices are reversed by loop.c into `subic.; bne`
  (r214 checkCatapult, throwRock `for (i = 10; i > 0; i--)`); `u32 i` is not (`cmplwi; ble`). A
  `do { .. d++; i++; } while (d <= &tbl[2]);` pointer walk keeps the pool `lfs` inside the loop and the
  `mr r29, r26` table copy (r214 initCatapult).
- Room-side `class cEmWrapD : public cEmWrap { ~cEmWrapD() {} };` for local `cEmWrap x[4]` arrays: the
  target has the ctor loop AND an empty dtor loop (`cmpw; beq; L: subi 0xc; cmpw; bne`, r214
  checkEmReset).
- Store order inside switch arms follows the "dying-first" rule the other way round from source order:
  `rockReady = 0; step = 2;` gives `stw step; stb rockReady` (r214 cCatapult214::move, three arms).
- COMPILER-DIFF 1 also shows in a *definition's* prologue (r223 reva_common_move: `fmr f28,f1; fmr f29,f2`
  before `mr r29,r5`); an asm-labelled definition cannot be assembled, so it stays a residual.
- Old prototypes the rooms were built against: `cEmRack::setBreak(Vec*)` (r227: `void
  cEmRackSetBreakV(cEmRack*, Vec*) asm("setBreak__7cEmRack")`), `SceAtDataSet_exec` 5th arg `(void*) 1`.
- Residual shapes left: fp-register order of 4 hoisted loop constants when pool order and priority order
  conflict (r227 checkBox0/1Fall, 14 words each); `&cMes.getWork()` pointer tied to r31 across the call
  (r227 operateElv, 2 words); `rot.x/y/z` literal store order z,y,x with the y literal first in the pool
  (r227 execGondola, 2 words); Init's inline `Vec*` param vs local Vec address (r213); FadeSetW register
  pair (r21d moveFence); PRE'd `&local` used through `mr r4,r31`/`4(r31)` stores in r214 execCatapult.
- `.bss` emission order: function-local statics (text order) -> objects needing construction (at
  finish_file) -> deferred plain file-scope statics/publics in definition order; `= 0` scalars go to `.data`.
- `global constructors keyed to X`: X is the first PUBLIC function emitted; a header prototype makes a
  `static`-defined function public -- remove it to move the key.
- Zero-aggregate initializer -> builtin memset libcall with `crclr cr1eq`; a prototyped `memset()` call
  never gets the builtin.
- `int off = !(flag & 1); if (off)` -> `xori; andi.; bne` (TRUTH_NOT as value).
- `(f32)(int)u8val` gives the lfd/0x4330 path instead of the `psq_l` fast-cast; `(u32)t` from float
  gives the `fcmpu 2^31 / bso / xoris` unsigned fixup.
- Post-increment in a loop condition (`i++ < n - 1`) yields `mr r0,rI; cmpw; addi rI` with `n-1`
  hoisted; `++i < n` yields the in-place biv compare.


### Weapon modules (wep00/34/35/36/37, wep11/12/27/29/39 Matching; wep13 pl_rocket 20/24; include/wep_mod.h, src/wep/, src/wepXX/; 2026-09)

- Every weapon REL is 2-4 objects, not one (config/G4BE08/modules.py UNITS; the `[cFlag.set()][atari.h][light.h]`
  header-string group repeats once per object and pins each object's `.rodata`): the player routine object of the
  weapon class (`PlXxxMove` + `wepNN_r2/r3_*` routines + the NAMED cManager<cLight> countActiveWork/create(int)
  copies; byte-identical in every module of the class, so one shared source `src/wep/pl_<class>.cpp`: pl_machine
  = wep11/12/27/29/39, pl_rocket = wep13, pl_handgun = wep01/02/04/05/06/15/38/43/44, pl_shotgun = 07/08/33,
  pl_rifle = 09/10/40/47, pl_knife = 16/26, pl_bow = 28, pl_grenade = 19/30/41/42/45), then the module object
  (`WepXX_init` [+ `equipWeapon`], the cObjXxx class when it is not a separate object, `ObjXxx_init`,
  `_prolog/_epilog/_unresolved`, the nameless 0x3B8 cLight block, the cObj countActiveWork/createBack copies,
  the cUnit copies). wep01/02/05/09/10/11/12/29/43 put the weapon class in its own FIRST object (wep02 has two:
  objMauser + objRuger), wep14/wep17 have no routine object. The five hand modules (wep00/34-37) are one object.
- `ObjXxx_init` scope follows the object layout: static when it sits in the module object (wep39/wep27: the REL
  field holds S+A), global when the class is a separate object (wep11/wep29/wep12: `_prolog` in another object
  stores it; the field is 0 either way because S = 0, so the .sym's `local` is wrong there).
- The module's `.data` section is 8-aligned in the original (`lockCtr` u8 at 0, the routine tables from 4):
  `asm(".section .data\n\t.balign 8\n\t.text")` at the end of the routine source, otherwise the REL's .data
  moves by 4. Each routine object defines its OWN `u8 lockCtr = 0;` (.data+0, a copy of pl_wep.cpp's) in the
  machine gun / rocket classes; the knife/grenade routines import the DOL's.
- Skeleton (include/wep_mod.h): `_prolog` = `WeaponInitFunc = WepXX_init; WeaponMoveFunc = PlXxxMove;
  ObjInitFunc[id] = ObjXxx_init; OSReport(...)`, `_epilog` = `ObjInitFunc[id] = 0`. `WepXX_init(cModel*)` =
  `createBack(id)` (or `equipWeapon`), err on NULL, `pl->pWep->pObj = obj; obj->init(pl); obj->setMotion(pl);
  EspDataLoad((u32) WEP_ARC_PTR(n), owner, 1); PlWepMot[0..2] = WEP_ARC_PTR(..)`. Player virtual slots used
  by the modules: setRightHand (vtable 0x68), setLeftHand (0x70); cObjWep::init (0x70), setMotion (0x78).
- `setMotion`: every `pl->pMotTbl[i] = WEP_ARC_PTR(n)` store is a `PSet(void*&, void*)` reference store (the
  original reloads `pG` AND `pl->pMotTbl` after each one; a plain `[i]` store keeps `pG` for the in-struct
  indices). `PL_ARC_PTR(pG->pPlArc, 0x5D)` for slot 0x3D, NULL slots as `PSet(.., 0)`.
- cObjXxx::init: the light `static const Vec p0/p1` are declared in a block AFTER the modelInit error (the
  string precedes them in .rodata); `AtariFlagsAnd(&sub2B4.atari, 0xFCFF)` (pointer form, `addi 0x2b4; lhz 0x1a`);
  `PSet(wep.parent, parent)`; the type switch is `case 0: default:` + 1/2/3 (`cmpwi 1; beq; ble default`) with
  `PSet(pMotNormal); PSet(pMotEmpty); wep.x24 = K; setAbility(..)` in that order; the ability constants are
  folded products of the cObjWep::init defaults (`5.73f * 0.7f`, `2.86f * 0.7f`, `0.2864f * 0.7f`, `* 0.2f`,
  `* 0.5f`). objTompson: `wep.parent = parent; U16Set(wep.x24, 0x34); wep.pMotNormal = ..; resetMotion();
  setAbility(5.73f, 2.86f, 0.2864f, 0.2864f)` (the multiplies are real code there).
- Routine bytes in the routine files: both plain `pl->xFC..xFF` stores and the int inline `PlRoutineSet(pl,
  r0, r1, r2, r3)` occur, block by block; brute-force with the /tmp harness (24 permutations + the inline,
  0.3 s each). Found: `PlRoutineSet` wherever an SI zero must not merge with a preceding QI byte store
  (wep13 r2_throw `wep.step = 0` before the call), the natural `fc, fd, fe, ff` order for most plain blocks,
  `RS(..); x3E0 = 0;` vs `x3E0 = 0; RS(..)` decided per block, and the `flags_420 & 0x40` else-arms need
  `int md = 3; PlRoutineSet(pl, 0, 6, md, 0);` (a local for the xFE constant puts its `li` first).
- `FACE_SET` (face blend reset) is a plain `{ }` block, not do/while(0): the loop notes flip the callee-saved
  order of `pl` vs a `joyLKamae()` result (wep13 down30). `if ((int) pl->x3E0++ > 9)` gives the `mr r0,r9;
  addi r9; cmpwi r0` post-increment (u32 field, signed compare). `int se = 0x28; if (..) se = 0x29;
  SndCall(1, (u16) se, ..)` gives the `clrlwi` (multi-set int). `const f32 endFrame = 5.0f;` at the function
  top orders the pool (pl_machine ready10). `cObjWep* obj = pl->pWep->pObj; obj->wep.mode = 5; obj->wep.step
  = 0;` (one pObj load for two byte stores; the following `pl->pWep->pObj->setMotion(pl)` re-reads it).
- OPEN (wep13 pl_rocket, 3 functions): r2_set keeps a callee-saved `&to` pseudo for the launcher line copy
  where the original recomputes `addi r5,r1,0x18` and reads `to` from the frame (16 words; direct call,
  inline-with-locals, pointer-parameter inline, memberwise copy, struct/array locals all tried); r2_next's
  last arm `li r9,1; li r0,6` order (24 permutations + inline tried); down00 `li r9,3` before the stack-arg
  `stw` of mot3.set (locals forms tried).
- `tools/casetree.py` models stmt.c `group/balance/emit_case_nodes` (validated against our compiler):
  give it the case values / default grouping and it predicts the compare tree -- use it to pin down
  damage-switch case lists instead of brute force.
- em3e is `emmark.cpp` (the disc's Bio4.emmark.sym describes it); a full `gen_rel_config.py` regen
  rewrites all 110 modules' files -- run it in a temporary copy of config/G4BE08 and install only the
  target module's files.


### t_esp/t_esp.cpp (the effect editor, 170/212 functions byte-identical, .rodata/.data/.bss identical, not Matching; 2026-09)

- Structure (src/t_esp/t_esp.cpp, `namespace t_esp_namespace`, include set = db_widget.h only: the .rodata header
  group is atari/light/map_obj/widget with no event.h string, so db_port's API is redeclared locally): 50 window
  structs `{DB_PRIM_ARRAY* pa; DB_WINDOW* win;}` built by IN-CLASS constructors that InitTool inlines
  (`g_pXWin = new X_WINDOW(g_pPrimArray)`), each widget in its own block `{ DB_POINT pos(x, y); int sx = N;
  pa->CreateButton(win, "..", &pos, cb, &sx, sy); }`. The `.rodata` proves the layout: every window's strings
  sit between its callbacks' strings/pools (parse order of the class body), InitTool's own pool (0x140 floats,
  first-use order = construction order) follows ClearSeqData's, so InitTool is one function calling the inlined
  ctors in .text order: Menu, Exit, Edit x4, Model, Load, LoadEm/Room/Sst/Event, Save, SaveEm/Room/Sst/Event,
  LoadCheck, SaveCheck, Option, DataSet, Time, ID, Path, Parent, Pos, Size, Speed, Color, Blend, Flag, Life, RT,
  AnmRate, Rotate, Vec0-2, Sub, Work0-6, WorkSp0-3, BasePos (the SUB_WINDOW class is DEFINED after WorkSp3
  although constructed before Work0; the LOAD_CHECK class right after LoadCheckClose_callback, before Save).
- Block-scoped `DB_POINT pos(x, y)` (db_widget.h gained `DB_POINT()` / `DB_POINT(f32, f32)`; db_widget/db_window/
  db_port unchanged) + `int sx` per widget are NOT slot-reused: the class-with-ctor temporaries get monotonic
  frame slots (16 bytes per widget, 0x2048 bytes of locals + one spilled `&local` pseudo each in InitTool's
  0x2930 frame). Same for inlined helpers holding such blocks. x stored via the frame, y via the ctor's `this`
  pseudo (`stfs f, 0x20(r1); stfs f, 4(rX)`).
- A public in-class inline ctor with a `static` local is never inlined (cp/decl.c "function with static variable
  cannot be inline"): `ID_WINDOW::ID_WINDOW` owns `static const char* kindName[] = {"Esp ", "Ctrl"}` (.data
  0x1524, defined right before the numeric that uses it, after the window's strings) and is therefore the only
  out-of-line ctor, emitted as a deferred inline AFTER the module's cManager<cLight> block (sym_map row renamed
  by hand to `__Q215t_esp_namespace9ID_WINDOWP13DB_PRIM_ARRAY`; sync cannot resolve linkonce copies).
- .bss order: EspToolTrans's unused `static DB_KEYBORD key;` (object + `_.tmp_0` guard) comes FIRST (function-local
  statics precede all file-scope ones), then the file-scope statics in declaration order (window pointers
  154D60..), the few globals (g_pMenuExitButton, the six pos/rpos DB_NUMERIC2*, g_immFlg[256]) interleaved at
  their declaration position. `.data` = 19 initialised scalars (`u8 = 0` stays in .data under -G 0), the name
  tables at the top (model 243 entries with two duplicated names, parent/parts 256, blend 17, filter 71, render
  9), then ID_WINDOW's table, ColorSimTypeUpdate's 16-entry table and three unreferenced statics
  (0, 0, 0x40000). `db_modelNo` (db_port.cpp .data+0x754, module label renamed by hand) is the BasePos "WorKNo".
- The record is the tool's own 0x12C struct (`TOOL_SEQ`: EspGenWork with array views x104[4]/x10C[4]/x110[4]/
  x124[4]/x128[4] and `Vec` fields); MakeImmSeq/AddSeq are 115-field macro expansions (`IMM(f)`: compare, flag
  byte, copy; `ADD(f)`: if/else store — the ternary form forwards the stored value and loses the clamp reload).
  s8 fields compare with plain `lbz/cmpw` (no extsb), s16 with `lha`. AddSeq's colour adds skip `no++` on the
  saturated paths (original bug, reproduced); the un-merged single `stb` of the 255 arm is COMPILER-DIFF #6.
- Idioms: `ISet(int&, int)` for `win->bring = 1` / `win->active = 0` in callbacks (the target reloads the next
  window pointer after the store); toggles are `if (v) v = 0; else v = 1;`; the dead `cmpwi 0; cmpwi 0xff` pair
  in the Save*FileNo callbacks is an `if (t < 0) .. else if (t > 0xFF)` on an unused s16 copy of the model type;
  `u32 i` edit-row loop (`(f32) i * 16.0f` is the unsigned double trick); PosStick scales 2000/-2000, 100/-100,
  40/-40, 1000/-1000, 1/-1 (fneg), 10/-10 with the y sign flipped for parts 0xF8..0xFD (`(u8)(parts + 8) <= 5`).
- Open: InitTool (-0x350: the 50 inlined ctors are written, register/`&local` spill order and `pa`/`p` choice per
  window not verified), ID_WINDOW ctor (254 words), Load/SaveEmTypeUpdate (group-skip loop shape), the
  0x12C struct-copy loops (DeleteSeq/InsertSeq/Copy/Paste/Make*SeqData: giv/base register choice), EspToolMain/
  ToolEspMain/DrawPosCursor (+-4..20), EditActiveChange (+4), the `bring`/`active` callbacks' zero register.

### Stage rooms, last-four pass (r212 Matching; r221 31/38 written, sections identical; r22c .data identical, 9/48 written; r226 not started; 2026-09)
- r212 (src/st2/r212.cpp, st2_2, MATCHING): a room-local class driven by a member-function table
  (`static void (cR212Door::*tbl[3])() = {&wait, &open, &close}` in `.data`: `{0, 0xFFFF, ptr}` PMF
  entries, called `(this->*tbl[mode])()` — a class without virtuals gets no index test); its
  `cEmGanado::setDrill` pointer copied into a call-clobbered `mr r8,r3` = ONE function-scope
  `cEmGanado* g` assigned in both `if` blocks (a per-block local uses r3 directly).
- `PSetSat(work->p, EatMgr.create(...))` with the call INSIDE the reference setter's argument hoists
  the `lis work@ha` into a callee-saved register across the call; `cSat* e = create(); PSetSat(work->p, e)`
  keeps the target's fresh `lis r11` after the call (TrapInit eat0). The later stores of the same
  function want the call-inside form (sat[0..3], sat2, eat, eat2).
- A template-copied `Vec pos` at frame offset 0 passed to two `setPos` calls in later blocks: the
  target's `mr r28,r11` (copy of the block-move address pseudo) + `mr r4,r28` per call comes from a
  block-local `Vec* pp = &pos;` declared in EACH block (gcse PRE hoists the redundant `(plus fp 8)`
  into a copy after the template copy); one function-scope `Vec* pp` is merged by cse/cprop into the
  block-move pseudo and a bare `&pos` recomputes `addi r4,r1,8` per call (r212 EventTrap).
- A `u32 cnt` whose `li` the target issues after the template copy's `addi r9` but before its loads:
  declare `u32 cnt;` at the top and assign `cnt = 0;` right before the loop (RoofMove); an
  initialised declaration is scheduled among the copy's loads.
- `(f32)` GPR constant: `f32 ry = -2.68f;` declared mid-block (after the module swap) lands as
  `lwz r29, LC` before the first setPos and is stored with `stw` (SF constant in a GPR); declared at
  the top it becomes an FPR (`lfs f30`) at the prologue (EventTrap); an FPR the target keeps across a
  call (`lfs f30` before `setPos`, stored after) is `f32 ry = 3.09f;` declared right BEFORE the call
  and after the Vec stores that precede it (DrillAppearCheckEndProc; the pool order follows).
- `(u8) prm[i][k]` of an `int prm[4][3]` template array passed to an int parameter gives the target's
  `lbz +3; clrlwi 24` pair by itself in the first loop (giv pointer `&prm[i][k]+3`), plain `lbz 3(rP)`
  in the second; `u8` locals and asm launders both lose the mask (RoofTrapWatcher).
- `(no >> 5) << 2` with an `int no` parameter of an inline is `srawi; slwi`; the target's
  `rlwinm 29,3,29` needs `u32 no` (event flag / door flag helpers).
- Byte pair `mode`/`step` stores after a word store: the target issues `stb step; stb mode` from the
  source order `mode = ..; step = ..;` (last dying store first: setOpen/setClose/setOpened/setClosed).
- `EmSeCall` in the rooms passes the position in the SECOND slot: `EmSeCallP(int no, Vec* pos, int id,
  ...) asm("EmSeCall__FiiP3VeciiP5cUnit")` (the DOL definition forwards it there).
- r221 (src/st2/r221.cpp, st2_3, not flipped): stack `cSceObj` elevator like r220; `switch ((u32) dir)`
  with cases 0/1 for the `cmpwi 1; beq; cmplwi 1; bge` dir tree (OPEN: ours emits a linear
  `cmpwi 0/1` tree and folds the `(Vec*) down` argument to `li r5,0` where the target keeps `mr r5,r31`
  — its `cmpwi down,0` is hoisted to the top and spilled `mfcr`, so cse never sees the compare).
  `Vec a = {3277.0f, o->pos.y, 4196.0f}` (a MEM element) builds the constructor in a freed temp slot
  (memset + 3 `stfs`) and copies it into the variable; `{0.0f, dy, 0.0f}` with a REG `dy` builds in
  place — the target of initShutter has both the temp copy AND the difference computed once before
  the memset (OPEN, 6 forms tried). Float constants: `-407.00006f` (c3cb8002), `0.15280247f`,
  `5.471593f`, `3.1518683f`, `-1793.51f`, `-1410.2101f`, `-1350.1799f` (read pool words, do not
  round). `ScePrim* t = SceExec(...); PSetPrim(work->x, t);` = the eat0 idiom for task handles.
  `emDeadWords(list)[4]` (inline returning `(u32*)((list << 5) + (u32) pG + 0x501C)`) keeps the
  target's `addi 0x501c; lwz 0x10(r9)`; the folded `lwz 0x502c` comes from the flat expression.
  OPEN: checkElevatorArrive (-8: the `k+1`/`7200*k/7` giv shapes), checkBossAppear_end / throwBonbe
  (r21/r22 global-alloc of `eff2` vs `pG@ha`), initShutter, moveElevator.
- r22c (src/st2/r22c.cpp, st2_4, partial): `.data` is 263 objects generated from the split object —
  every target record is its own `static s32 name[]` array of varying length and the level tables
  are `static s32* lvl[] = {(s32*) count, (s32*) time, rec, ...}`; `.data` needs `.balign 8` at the
  end. The level names are a `static const char* [5]` in `.data` right after the master table (its
  strings follow the r22c.cpp filename in `.rodata`), the routine table `void (*[5])()` and a
  `const char* = "START"` variable come after itemSave (string order), then a record, `int = 4`, and
  the `char fname[] = "SS/___/id22c.dat"` the code patches. 9 of 48 functions identical; the shooting
  game (getBonus .. r22c_checkShootingScore, ResultScreen, Score*) is NOT written (static stubs
  keep the table relocations; replace them).
- GCC 2.95 memory disambiguation (confirmed): a struct-member store never blocks a scalar global load
  (`pG`); a store through a reference parameter (`PSet`/`BitOn16`/`U16Set`) or a scalar pointer deref
  blocks everything -- choose per site from whether the target reloads `pG` after the store.
- `*(volatile u16*) &at->flags |= m` keeps `addi rX,obj,0x2b4; lhz/sth 0x1a(rX)` AND keeps the following
  `pG` load below it (`AtariFlagsOr`).
- Among independent stores sharing one value register, the later use is issued early and the earlier
  use sinks before the call: to make a store land last, write it first.
- `int md = 1; obj->wep.mode = md;` gives an SImode pseudo cse reuses for a later `x = 1` store in the
  same ebb; a QImode constant is not reused for a later SImode store.
- Identical if/else arms calling the same function with constant args are not cross-jumped -- write
  them duplicated.
- HAZARD: two agents editing config/G4BE08/modules.py concurrently -- one read-modify-write dropped 14
  MATCHING entries (a446c30). Re-read the file immediately before editing and re-verify RELs after.


### Stage rooms, r22c shooting-game pass (st2_4/r22c 48/50 byte-identical, sections identical, not flipped; 2026-09-10)
- src/st2/r22c.cpp is complete (all 48 functions + the cManager<cObj>/<cEm> copies). The work pointer is
  the one-member struct `R22cWorkPtr r22c_work` (every `r22c_work.p->x = v` reloads the pointer, as the
  target does after each store); the previous plain `static R22cWork*` was why 9 "matched" functions
  and checkBottleCap/getBottleCap did not match. Work layout: step/resultStep u8 at 0/1, timer 4, time 8,
  hits C, score 10, total 14, ageSum 18, level 1C, state 20, combo 24, pause 28, ufoWait 2C, shotTotal/
  shotHit 30/34, eat 38, wepSel 3C, effFlags 40, wepNo/wepType/cnt46/cnt47 44..47, tbl 4C, itemSaveBuf 50,
  itemSel 54, cap[24] 58, capId B8, door[2] BC, strId C4, wepMan C8, effTimer CC, itemNum[24] D0,
  ResultScreen {state, data} 130, IDSystem 138, scoreTimer[8] 188 (0x1A8 total).
- Linkage: getBonus, r22c_checkGameLevel, r22c_checkGame, countMark, funcUfo (static), deleteAllMark,
  scoreRegist, setWepmanKilled, ScoreClear, ScoreMove are `extern "C"` (the .sym shows them without
  `()`); ResultScreen::{read,reloadtime,highscore(int),init,move(int),quit} are C++ members, renamed by
  the sync (`move__12ResultScreeni` etc.).
- `.rodata`/`.data` order: `static const char* r22c_levelName[5]` must be DEFINED AFTER R22cInit (its
  "-","A".. strings follow the r22c.cpp file-name string emitted by R22cInit's MEM_CALLOC); the routine
  table + `static const char* r22c_startMsg = "START"` sit between shootInit and shootReady (the START
  string follows shootInit's 0.0 pool); `static const int r22c_scoreTbl[7][5]` (file scope) lands after
  the cManager strings.
- Idioms found: `int id = tbl[k]` local before `ItemMgr.num((u16)(id + 0xDB))` keeps `lwzx` (the inline
  expression narrows the load to `lhz +2`); a `for (i = 0, n = 0; ...)` comma init gives `li i; li n`
  in that order (separate `n = 0;` before the loop puts it first); `pG->x8330` high scores are read
  through a `struct { u8 pad[0x8330]; s16 score[4]; }` view of pG (`lhax base,idx` with the record
  base first; the `(s16*)&pG->x8330` cast gives idx-first `add`); `return RsfCheck(..) == 0` is the
  `and.; mfcr; extrwi` store flag; an if/else on `(int) flags_174 < 0` whose else arm is one call that
  also ends a switch case is `goto` into that case body (COMPILER-DIFF 6 shape, r22c_talkWepMan);
  `switch (d->flag) { default: ... case 0xFE: ... case 0xFD: }` (default first) for the linear
  `cmpwi FD; beq; cmpwi FE; beq` chain with the default body laid out first (shootMain); the cap-total
  digit display accumulates into a separate `sum` and copies it (`mr r31, r30`) before the digit
  code (ResultScreen::move); ScoreSet's leading-zero count is a goto loop over `int* p = &d[3]` with
  `*--p` (`lwzu -4`) and `n = 1;` written before the pointer init; the digit array is walked through
  `int* d = digit` (`lwz 0xc(r7)` instead of the frame-relative form); loop-local `IdUnit* du` per
  display loop keeps the call result in r3 (a reused function-scope `u` gets a callee-saved copy);
  `(w->level == 2 || w->level == 3) && w->state == 2 || (w->level == 4 && w->state == 1) ||
  (w->level == 4 && w->state == 2)` as a MACRO (an inline returning it materialises `li/cmpwi`) gives
  the range fold on level and the two separate state compares (cse deletes the repeated level test);
  `flagBit(f, 1) && !(f & 2)` keeps two `andi.` where `(f & 1) && !(f & 2)` folds to `clrlwi 30; cmpwi 1`.
- Residuals: r22cGateCtrl (18 words: the target schedules `li open,0` before the first `init` call and
  the loop-invariant `lis` fillers one call earlier each, so `pG@ha` gets its own r24; every placement
  of `open = 0`, lim/spd/type forms tried); ResultScreen::highscore (10 words: `score` param r28 <->
  loop `IdSys@ha` r31 global-alloc order; `on` reusing `score` gives the target's r28 for the flag).

### Small tool RELs, flags-first sweep (t_scroll 38/42, snd_test 68/77, t_esp 186/212; none flipped; 2026-09-10)

- Judge with a masked compare whose `.rodata` reloc targets are compared by *content* (the referenced
  bytes), not by offset: snd_test's one missing pool word (the `Snd_voice_work` spill, original-only)
  shifted every later string and made 14 byte-identical functions look different. Placeholder-named
  target functions (`menu_13488`) pair with ours by `.text` order only when both objects list the same
  function set; sync the names first or the pairing lies (t_event today).
- `while ((s8) *p != '\n' && (s8) *p != '/') p++; if ((s8) *p != '\n')` — the second test in the SAME
  mode (`(s8)`) as the loop's exit compare lets jump.c thread the loop's `'\n'` exit past it
  (`beq -> loop end`); combine then narrows the single-use compare back to `lbz; cmpwi`. A `u8` view
  (`*p != '\n'`) keeps the re-test (t_scroll loadBinName).
- `lbzu rX,3(rP); cmpwi rX,'\n'; ... extsb` (no `clrlwi`, extsb only for the switch) is
  `p += 3; while (*p != '\n') { switch ((s8) *p) {..} p++; }` — the value re-read per iteration; a `c`
  variable (any type) that feeds both the compare and the switch gets an extra extension.
- `lbz r0; mr r31,r0; stb r0` (loaded byte copied into a long-lived variable, the store from the load
  register) is a *read-back of the just-stored member*: `obj->x = pWork->id; id = obj->x;` (cse forwards
  the store as a plain `mr`); `int id = pWork->id; obj->x = pWork->id;` gives `clrlwi` (t_scroll edit_ot).
- `lbz r0,40(r3); extsb r0; slwi r0; add r3,r0,r9` with the index temp in r0 in every arm of a
  function returning `idx * size + base`: ONE result variable assigned in each arm and a single
  `return p;` — the result pseudo is global, so local-alloc cannot tie the dying index to r3 (direct
  `return` expressions tie it: `lbz r3; ... add r3,r3,r9`) (snd_test test_get_tpara_adrs).
- `li r3,1` before the compare + `bne end` skipping a shared out-of-line `li r3,0` = `if (a != b)
  return 1; return 0;`; the `== b` form puts `li r3,0` inline and jumps to a trailing `li r3,1`
  (test_tbl_para_select). Same family: `beq body; li r3,1; b end; body..; li r3,1; b end; li r3,0` =
  an early `if (x != old) return 1;` plus a tail written `if (v == cur) return 0; store; call; return 1;`
  (the success block is the fallthrough, the fail `li r3,0` sits out of line at the end) (Snd_test_volume).
- A member RMW whose store is unconditional while only the update is guarded (`lhz` at the function
  top, `beq skip; xor; skip: sth`) is `v = *p; ...; if (c) v ^= bit; *p = v;` (move_type_flag).
- `lbz r3,3(r31); extsb r9,r3; lbzx ..,r9; ...; mr r3,r9` (raw byte in r3, sign-extended index in r9,
  copied to the call argument) = the s8 member read DIRECTLY at both uses (`w->efxState[w->aux]`,
  `Snd_efx_req(w->aux, ..)`): cse shares the QI load and the extension is a separate pseudo. A
  `s8 aux = w->aux` local ties the extension to r3 (test_efx_on_or_off).
- `lis/addi Snd_ctrl_work` hoisted into a callee-saved register before earlier calls, member stores
  `stb/sth off(rX)` = a `SND_CTRL_WORK* ctrl = &Snd_ctrl_work;` local declared before the calls
  (test_play_or_stop, Snd_test_disp_vol, Snd_test_disp_req_para). Snd_test_disp_req_para ignores its
  parameter: `SndTestWork* w = &Snd_test_work;` (the `lis/addi Snd_test_work` in r30).
- A function-level `int n = 0;` whose `li` the target issues AFTER a preceding loop is `int n; ...
  n = 0;` assigned after that loop; `int step = 1` likewise assigned after the early return / after
  the value it is compared against (test_tbl_aux_ck, Snd_test_volume, aram_dump_disp).
- `x` column as an explicit variable (`int x = 0x68; ... x += 0x18;` re-initialised per outer
  iteration) instead of the `0x68 + j * 0x18` giv: the counter keeps the higher register (target j r31,
  x r30) and the increments come out `j++, x += 24, n++` (aram_dump_disp).
- `v &= 0x7F; *p = v;` ties the mask to the variable's register (`clrlwi r9,r9,25`); `*p = v & 0x7F`
  gives a fresh `clrlwi r0,r9,25` (test_tbl_aux_ck).
- s8 fields the target `extsb`s in an eprintf `%d` argument while the header declares them u8:
  cast at the use (`(s8) sit->se_flag`), do not change the shared header (disp_sit_midi).
- A dead string in `.rodata` between two functions' string groups with no code using it: an `if (0)`
  call with that literal at the end of the preceding function reproduces it without STRIP_UNUSED
  (snd_test blk_file_disp "sbb").
- Pointer-global stores followed by a `->member` load of another global (t_esp callbacks
  `g_pLoadNow = NULL; g_pSaveNow = X; BRING(X)`): BOTH stores through a pointer-reference setter
  (`WSet(TOOL_WINDOW*&, ..)`, incl. the NULL one) keep the `lwz 4(rX)` below them AND in source
  order; with only the second one a reference the dependent store is issued first. `ISet` on an int
  global (`g_dataChanged = 1`, `g_modelLoad = 1`) before `BRING(g_pMenuWin)` likewise. `ISet(w->active,
  0)` on the parameter window keeps the `lwz g_pEditWin1` below it (EditActiveNext/PrevWindow).
- Nested call as an argument: the original re-reads a memory operand argument AFTER the inner call
  (`bl inner; mr r5,r3; lwz r4,0(r30)` with only the address `lis` callee-saved) — sequence the inner
  call into a local first (`int num = MakeSaveSeqData(..); SaveData(path, g_pSeqHead, num)`); ours
  precomputes the operand into a callee-saved pseudo before the inner call (SaveCheckOkCallback).
  OPEN variant (snd_test test_play_or_stop): the target has a DEAD `lhz r4,32(r31)` before the inner
  call and the reload after it — no source form gives the dead load (2 words, kept nested).
- Two identical if/else tails `r = g; b = a;` shared after an inner if/else (`b d8` into the sibling
  arm's copies): write the tail ONCE after the inner if/else, not per arm (SetEditTblColor; the
  remaining `lfs f28/fmr f30` vs `lfs f30/fmr f28` is cse's (set REG0 REG1) swap on `g = 1.0f; a = g;`
  — a's REGNO_LAST_UID is later than g's; no join order gives both the target's copy direction and
  copy order).
- `mulli r0,row,43; slwi r0,r0,2; add base` for a `[5][43]` table row loop = a FLAT index
  (`g_editNum[0][row * 43 + i]`); the 2-D `g_editNum[row][i]` folds to `mulli 172`.
- `sel->selX == 3 || sel->selX == 4` with two `cmpwi/beq` in the target: `SelXIs(sel, 3) ||
  SelXIs(sel, 4)` inline compares (the SysRegionIs idiom; the lvalue form is range-folded).
- `.bss` order of two pointer globals (`g_pLoadDirButton`/`g_pSaveDirButton` swapped): first
  declaration order, visible only through the reloc offsets of a function that reads both.
- Init store blocks derived in one shot with the refined store-order rule (ClearSeqData, 14 stores):
  target issue order = [the LAST dying store in RTL] + [the other dying stores in RTL order] + [the
  non-dying stores in RTL order]; a constant shared by several stores dies at its LAST store. Read
  the RTL order off the target: parts(0xFE), w, h, dplus, x30, anmRate, r, g, b, a, dr, dg, db, da
  with r/g/b/a and dr/dg/db/da as SEPARATE statements (a chain reverses them).
- OPEN, compiler-side (do not retry): edit_select_sub — gcse PRE turns the three post-loop
  `high(scrollWorkPtr)` into copies of a bb0 pseudo, the post-PRE cprop pass then substitutes that
  pseudo into the skipped-block join (`lo_sum 193`), cse2 restores the `lis` in every block and the
  join keeps its own pseudo (fresh `lis r9`); the target shares one `lis r10` between the `& 0x900`
  block and the `& 0x200` join, i.e. its cprop did not touch the join. DbSctrl `w->x/w->y` order:
  sched1 gives the target order (y, x), sched2 flips it through the anti-dependence of `stw r4` on
  the later `lwz r4,pG` (the clamped anti-dep cost rule). Snd_test_disp_voice / toolIdSpace:
  interblock speculative motion of the arm's arg moves / of the `&&` second compare into the test
  block — ours moves in disp_voice where the target does not, the target moves in toolIdSpace where
  ours does not (COMPILER-DIFF #5 family). Save*FileNoUpdateCallback (t_esp, 5 x 5 words): the dead
  `cmpwi 0; cmpwi 255` pair — ours keeps only the first compare of the dead `if (type < 0) ..
  else if (type > 0xFF)` (flow deletes the second store first, jump2's delete_computation removes its
  compare); switch/ternary/two-if/int forms all keep one compare or a branch.
- Not moved (register/schedule only, forms tried in the sources): t_scroll edit_litmask (`li 1`
  before `lwz x54`: both issue at the same cycle, unit choice), loadBinName tail `addi p` vs
  `cmplwi num`, printEditTable (112 words, allocation); snd_test test_blk_enable_ck (three
  callee-saved invariants vs r3/r0), dir_entry_read (gcse PRE of `&w->dir` into the loop's first
  block marks the loop phony: no invariant hoisting; the target hoists `cmpwi cr4,dirs` and the
  `blk_ext_name`/`dirName`/`dirIsDir` addresses — a `DVDDir* dir` local or moving `name[]` does
  not stop the PRE), disp_sit_normal/disp_sequencer/snd_test_disp_rit (original-only shapes, see
  the fourth-pass notes); db_sctrl and t_camera_data residues are the ones listed in their sections
  (tcDataImport: `extsb r7,r11` untied index + `lis pLog` not hoisted out of the record loop).

### Weapon modules, wave 3 (wep14 written + Matching; module objects of wep01/04/06/13/17/19/30/38/41/42/43/45 flipped; pl_shotgun 17/18, pl_rocket 23/24; 2026-09)

- Every weapon module is Matching except the three pl_shotgun copies (wep07/08/33, `wep07_r3_fire00` 2 words)
  and pl_rocket (wep13, `wep13_r3_down00` 2 words); both residues are scheduler tie-breaks, see below.
- wep14 (mine thrower) = `wep14/objMine.cpp` (cObjMine: init/moveReady/moveFire/setBullet/moveDown/moveReload/
  setCartridge/interrupt/setMotion + the free `partsSet(cObjMine*)`) and `wep14/wep14.cpp` (its own copy of the
  handgun routine with a scope aim type, `wep14changeRightHand`, `equipWeapon`). Idioms found there:
  - A module object that includes light.h but whose REL has no cManager<cLight> block (wep14.cpp, wep17.cpp: the
    header string is in `.rodata`, the 0x3B8 block is not in `.text`): list the five instantiations in modules.py
    `LINKONCE_DROP` (the linkonce rule keeps unnamed copies when no unit of the module names the class instance).
  - `!(pG->wep_type & 1)` in a `int normal = !(...); if (normal)` local gives the `lbz; xori 1; andi. 1; beq`
    shape; a direct `if (pG->wep_type & 1)` is the plain `andi.`. The shape is shared by setBullet / ready00 /
    r2_set, so the original had the negated test in a variable (or an inline returning it).
  - `cAtariInfo::init` with immediate ints and constant floats: `atariInitF(at, 0.0f, 100.0f, ...,1, 0, 0)`
    (include/atari_init.h, COMPILER-DIFF #1) reproduces the `fmr f5,f2; li r5; fmr f6,f5; mr r3; fmr f3,f1; li r4;
    fmr f7,f6; fmr f4,f3` chain; keep `cAtariInfo* at = &sub2B4.atari` for the following `AtariFlagsAnd(at, ..)`.
  - `U16Set(wep.x24, 0x36)` before modelInit (the `sth` precedes `lis pG`), `PSet(wep.parent, parent)` before the
    `PSet(wep.pMotNormal, ..)` (the `stw parent` precedes `lwz pG`): reference stores, like the setMotion table.
  - Three static Vecs with a `Vec* pos` chosen per branch (setBullet): the `pos = &minePos;` assignment must sit
    between `PSVECAdd(&minePos, ..)` and `PSVECSubtract(&mineAt, &minePos, &mineDir)` with the literal `&minePos`
    written everywhere (reload_cse turns the later lo_sums into `mr rX,r26`, the earlier calls keep fresh `addi`s
    and the `lis mineDir@ha` is issued before GetWepTargetPos); with `pos` used in the calls the &hit PRE copy and
    the `lis` swap slots (38 words), with `pos` assigned at the end the address is rematerialised (31).
  - `VECNormalize(&v, &v)` on a static Vec: `#line 212 "D:/Bio4/Prog/objMine.cpp"` right before it (the macro's
    `__LINE__`); `.x` through `sym@l(rHigh)`, `.y/.z` through one `addi` pseudo, the fail arm's `.x` store with a
    fresh `lis` -- all natural.
  - `static const f32 reloadFrame[2] = { 74.0f, 58.0f };` declared in the else block of moveReload emits the
    table twice in `.rodata` (0xF0 and 0xF8, the second referenced) -- our compiler does it too, no action.
  - `if (pG->x4FBA == 1) A else B` where the `!= 1` arm falls through (`cmpwi 1; beq L; [B]; b; L: [A]`): write it
    as a `switch (pG->x4FBA) { default: B; break; case 1: A; break; }` (objVp70 form) for both the motion pointer
    and the `se` constant (`li r4,0x20; beq; li r4,2`).
  - `obj->wep.mode = K; obj->wep.step = 0;` pairs come out in every order: `mode = 3; step = 0` -> `stb 34f;
    stb 34e` (r2_ready, wep17 wepDown), `step = 0; mode = 4` -> `stb 34f; stb 34e` too (r2_reload: both die),
    while r2_down's `stb r10(3),34e; stb r0(0),34f` needs `int md = 3; obj->wep.mode = md; obj->wep.step = 0;`
    (pl_knife's `int on = 1` idiom: an SImode constant for one of the two byte stores) -- `U8Set(step, 0)` with a
    promoted parameter works as well. Brute-force the four forms per block.
  - Two zero stores followed by a `pG` load that must stay below them (r2_next case 0: `stw 3e4; lis pG; stw 3e0;
    ...; lwz pG` after the `li r6`): `U32Set(pl->x3E0, 0); IntSet(pl->x3E4, 0);` (scalar-reference stores; the
    plain stores let the load float between them). wep17's r2_next (`stw r29,3e4; cmpwi; stw r29,3e0`, both
    from the switch register) only needs the source order `x3E0 = 0; x3E4 = 0;` (dying-first).
  - `FSet(pl->pWep->x30, CamCtrl.getCameraDirection())` when the next statement loads `pG`
    (wep14changeRightHand's WEP_ARC_PTR): the plain member store lets the `lwz pG` move above the `stfs`.
  - `AtariFlagsAndV(WEP_ATARI(pl), 0xFDFF)` (wep_mod.h, volatile like AtariFlagsOr) for a `clrFlag200()` that is
    followed by a `pG` load (r2_down); the plain `clrFlag200()` hoists the load above the `sth`.
  - Fall-through case bodies: wep14's r2_reload `case 0:` runs into `case 1:` (no break; the target's case-1 code
    follows the mode/step stores directly), moveReady's `case 0:` falls into `case 1:`.
  - The `flags_420 & 0x40` else-arm `li r9,1; li r0,6` order is the `int md = 1; PlRoutineSet(pl, 0, 6, md, 0);`
    idiom (also fixed pl_rocket r2_next and r2_set's `md = 3`); it fails when the same constant is stored later
    in the function (pl_rocket down00: `int hokan = 3` is cse'd into `pl->xFF = 3` and kept in a callee-saved
    register).
  - Class-object scope: `ObjMine_init` is global (the .sym's `local` is wrong: `_prolog` in the other object
    stores it); `ObjXxx_init` must be DEFINED before the class's first member (wep04/wep06 had it after
    setMotion, which reorders `.text` although every function matched: check `unit_info` offsets before flipping).
- pl_shotgun `wep07_r3_fire10`: the dead `cmpwi r0,0x21` after the tree comes from a THIRD case value below 8
  grouped with the default (`case 8: ..; case 0x21: endFrame = 0x28; break; case 7: default: endFrame = 0x28;`):
  the 3-node tree emits `beq L8; bgt test; [7: -> default]; b D; test: cmpwi 0x21; beq L21`, jump1 turns the
  vanished 7 leaf into `ble default`, jump2 cross-jumps the separate 0x21 body into the default and deletes
  only the `beq` (no cc0). Two case values alone give a linear list (`cmpwi 8; beq; cmpwi 0x21; beq; b`) whose
  0x21 compare our jump2 deletes together with the branch (10 forms tried before the tree was recognised).
- pl_rocket `wep13_r2_set`: the launcher line copy `obj->launcher.to = to` after `getTrajectory(&from, &to)`
  reads `to` straight from the frame (`lwz 0x18(r1)..0x20(r1)`) in the original while `from` (frame offset 0)
  goes through `addi r10,r1,8`: the call's `&to` is precomputed into a pseudo (a `(plus vsv 16)` argument),
  a plain struct copy makes cse reuse it for the copy's addresses and gcse PRE hoists it into a callee-saved
  register. Copy through `static inline void VecCopy(Vec* d, const Vec* s) { *d = *s; }` (the inline's pointer
  parameter is substituted by integrate and never joins the cse class): 16 -> 0 words.
- OPEN pl_shotgun `wep07_r3_fire00` (2 words): after PlWepLockRand the original issues `lfs f0, 0.0` before
  `lfs f13, m3r[2]` (`fcmpu f13,f0`), ours the loads in RTL order. Both loads have weight 0 in sched1 (`&m3r`
  dies at the m3r[2] load because the `m3r[1] = pitch` store precedes it in RTL), equal priority, equal class and
  dependents, so LUID decides; the target's 0.0 is nevertheless allocated f0 (the shorter-range qty), which
  contradicts a plain sched1 swap. The 0.0 pool word is shared with the loop's un-hoisted `p1.y/z = 0` loads
  (all 18 callee-saved FPRs hold loop constants). Literal/`0.0f ==`/`f32 zero` (right order, f0/f13 swapped)/
  `f32 z = m3r[2]`/`m3r` pointer/FSet/if-else duplication tried. pl_machine's identical tail (no loop) matches.
- OPEN pl_rocket `wep13_r3_down00` (2 words): `mot3.set(pl, mot0, mot0, mot0, (int) mot1, 3, 0, 4, 0)` issues
  `li r9,3` before the stack-argument `stw r0,8(r1)` in the original; both are true/anti dependences of the
  call with cost 1 (SN's insn_cost clamps anti-dependences to 1), so ours prefers the store (register weight
  -1 vs +1). The stores rank between `li r9` and `mr r6/mr r7/li r10` in the target, which no single
  rank_for_schedule change explains; `int/u8 hokan = 3` locals are cse'd into the later `xFF = 3` (13 words).
- mcmp.py note: after `sync_rel_symbols.py` the remaining `_prolog`/`equipWeapon` "2 words" of a module object
  are cross-unit reloc *names* (the split object's placeholder vs our mangled name, e.g. `PlHandgunMove` vs
  `PlHandgunMove__FP7cPlayer`; undefined symbols are not demangled by the tool) -- the REL shasum is the judge.

### DOL register-tie pass with the em10 levers (pl_class, lightPath, rnd, emBar, vfprintf Matching; 2026-09-10)

- Harness /tmp/dolties (`mcmp.py UNIT [SYM]` masked compare of `build/G4BE08/obj/<unit>.o` vs ours or `OBJ=`;
  it folds `GXWGFifo` relocs into the absolute word, accepts a raw `lis rX,0x80NN` the split object left
  unpaired against our `@ha` reloc, names `_GLOBAL_.I/.D` = `global_constructors/destructors`, compares
  data sections with relocated fields masked; `tryv.py UNIT SYM variants.py` builds source variants privately
  and prints the word count; `dump.sh UNIT -dX` = cpp + cc1plus dumps with the DOL flags, no `-G`).
- `do { stmt; } while (0)` as a pure allocation lever (loop notes: no code, the refs inside count one more in
  REG_N_REFS for BOTH local-alloc `qty_n_refs` and global-alloc `allocno_n_refs`; the notes are also a sched1
  barrier, so pick a statement whose neighbours are already ordered by dependences):
  - lightPath movePath: around the LAST `pCur = pCur + 1` store (`this` 9 weighted refs > pCur 5/14 -> this r9,
    pCur r11); the same wrap on the first arm's store or on the `v = pCur->data[0]` load reorders the code.
  - emBar emBarHitCk: around `emBarSetBreak(em, 2)` (or `em->dmType = 0`, or the `hp <= 0` test): 7th em ref
    (1686 > p 1509) -> em r31, p r30.
  - vfprintf fftoa: `do { u.d = value; } while (0); do { lo = u.w[1]; hi = u.w[0]; } while (0);` -- the second
    block makes lo (3 refs) beat sgn in global-alloc (lo r11, sgn r10), the first is the barrier that keeps the
    `sign` copy `mr r25,r4` ahead of the union's `fmr f0,f30` reload (that order alone was the 2-word residue).
  - room_jmp getRoomInfo: around `ofs = (p + 1)[stage]` (or around the `n` load): local-alloc then takes ofs
    first (r0), n r11, and global-alloc gives `base` the freed r0 (`add r0,r3,r0`).
  - Where it does NOT work: obj00 FallMove (the hit loop's QI `1`, 3 refs/82 vs the k loop's `k+1` giv 4/46 =
    1739; nested do-whiles around `w->sePlayed = 1` also raise `w`'s weight and move r25/r26), read readEmData
    (`m->flag |= 4` / `|= 1` in a do-while flips m/newSize to r29/r28 but the notes reorder the adjacent
    block: `cmplw; lwz pArc; ori; sth` becomes `ori; sth; lwz; cmplw`, or the MEM_ALLOC `li r5,__LINE__`
    moves -- 4 words either way), pad PadRead (the `&Pad_data` lo_sum pseudo has no statement of its own to
    wrap; `dead` 5/269 vs `&Pad_data` 4/227 with both REG_EQUIV-doubled), esp45 HideCheck, datactrl.
- Cross-jump survivor without COMPILER-DIFF 6 (pl_class isKamae, now Matching): find_cross_jump lowers its
  minimum to 1 when the insn before the scanned tail is a CODE_LABEL, so a `return 1` copy preceded by the
  join label of an `a || b` return-0 test is merged into the LAST identical copy. Writing that test as
  `if (a || b) goto ng;` with `ng: return 0;` being the function's final return puts `beq ng` twice with no
  label before the `li r3,1`; the scan of that copy then fails (1 insn, no jump-around-jump), the later copies
  cross-jump into each other and finally into it (`b kamae` / `bne kamae`), exactly the original layout.
- cse equivalence class breaker (rnd Rnd, now Matching): `u32 m = (n << 16) >> 16;` (n < 0x10000) keeps m out
  of n's class -- cse does not fold the shift pair, combine reduces it to `mr r0,r9` after cse, and the copy
  survives because n is still needed for `n + 0x101`. (`n & 0xFFFF` / `(u16) n` give the copy but swap the
  `clrlslwi`/`clrlwi` pair; the AND is folded by cse through nonzero_bits-free simplify_binary_operation.)
- .bss order: `MesData` was emitted before `MesFont` because global.h/mes.h declare `extern MesData` before
  mes.cpp's static; a forward `class MessageFont; extern MessageFont MesFont[4];` above the includes (and a
  non-static definition) restores cMes, MesFont, MesData, MsgQueue. mes is 71/73 (move `mr.`, WidthCk).
- Sched1 issue-order residues that the levers do not reach (all 1-3 words, unit otherwise byte-identical):
  - objWep drawPoint: the QI `0` for `esp->xA7` must sit BEFORE `stb 0xa4` at local-alloc time (then the four
    `esp` reloads get r11/r9/r11/r9 via the fake lifetimes: 154 fake death 12 overlaps 156 fake birth) while
    the final order is `stb; li r7,0; lwz r9` (sched2); ours issues `stb` first in sched1 (priority: the
    stb heads the mem-dependence chain of the following reloads). do-while/int-zero/order forms tried.
  - filter06 cParticle06::move: the `(u8) a` loadaddr pseudo must be issued below `lbz alphaBase` in sched1
    (then it has the shorter range and takes r9, alphaBase r11); ours ranks the loadaddr first (longer chain
    through psq_st/lbz/clrlwi).
  - esp45 HideCheck: the `i*4` giv init `li r30,0` (LUID after the movables) is issued after the hoisted
    `lis r26,Screen@ha` here and before it in the original; an explicit `u32 ofs` variable puts the `li`
    first but swaps hidden/ofs to r30/r29.
  - datactrl dispDebug: x1 (pseudo 106) inherits a preference for r7 from the local-alloc'd `x1 - x0` temp
    (set_preference on the MINUS operand: reg 199 = r7 counts as a hard reg) and takes it; the original's x1
    is r5 (`subf r7,r6,r5`), i.e. that preference was not applied there.
  - exception ErrorHandler: max_cuid at gcse time is 503 (251 buckets: `symbol_err_tbl` 72 < `.LC64` 194);
    255 or 257 buckets (508..515 real insns) or a `.LC70/71/73` label would flip the two PRE pseudos.
    Duplicated call tails are not cross-jumped back (flow's `use` after a block-ending CALL differs), the
    `-Joy.ssx` neg trick adds only +2, and a `if (n != 0)` dead block was not const-propagated by gcse.
  - obj00 FallMove: B (`w->sePlayed = 1`'s QI 1) takes r24 in pass 0 because A (`k+1` giv, allocated first)
    already used r24 and does not conflict; the original's B in r23 = C's register (the k loop's SI 1), so
    there A was allocated after B or r24 conflicted.

### Stage rooms, st1 r100/r117/r11c pass (r100 Matching; r117 20/21, r11c 21/23; r103/r104/r108/r118/r11d/r11e/r113/r120 residues and sections; 2026-09-10)

- r100 (st1_0, 37/37 byte-identical, sections identical, Matching; st1_0.rel byte-identical). r117 (st1_3,
  20/21, sections identical), r11c (st1_3, 21/23 real: closeGate is a #9 loop, EventBesiegedStart differs
  only in the `cManager<cEm>::create` reloc name and the two template instances are name-only) written.
- REL `.data`/`.rodata` alignment: the split object's section alignment must match ours or every later
  unit's data shifts (st1_0 r100: our 4-aligned `.data` moved the REL's .data start by 4, `st1_0.rel: FAILED`
  with every function identical). GNU as pads the section END to its alignment, so the alignment also
  shows as a size diff (r103 `.data` 0x24 vs 0x28, r104 `.rodata` 0x2a4 vs 0x2a8, r113/r11e/r11d `.data`).
  `asm(".section .data; .balign 8");` (or `.rodata`) before the first object of the section; check the
  target's `readelf -S` Al column (r100, r103, r104, r113, r11d, r11e now match). The reverse: r120's
  `.rodata .balign 8` was WRONG (the split object's align 8 is inferred from the address; st1.cpp's
  `.rodata` follows at 0x13b4, 4-aligned) — removed, `.rodata` 0x2c4 now equal. Check the next unit's
  offset in modules.py before trusting the split object's alignment.
- Unreferenced `.data` zero words: GCC 2.95 puts `static int x = 0;` in `.data` (r100's `r100_sndTimer`
  after `r100_sndPos`, 0x34 -> 0x38).
- Stack `cEm` in a room (r100 R100Init, r103 r103_setCorpse; em.h's cEm is the 0xDE0 EmMgr stride, the
  original's stack object is 0x3E0): `class RxxxEm : public cModel { u8 pad_320[0x378-0x320]; PlArc*
  subArc; u8 pad_37C[0x3E0-0x37C]; RxxxEm() asm("__3cEm"); };` with `RxxxEm em; RxxxEm* pe = &em;` and
  the store `*(PlArc**) ((u8*) pe + 0x378) = ...` — the original addresses subArc through the ctor's `this`
  pseudo (`addi r3,r1,8; mr r29,r3; ... 0x378(r29)`), a member access on `em` folds to the frame. The
  implicit `~cUnit` is emitted at scope end: do NOT add an explicit `((cUnit*) &em)->cUnit::~cUnit();`
  (double dtor). HEADER DEBT: plain `cEm em;` once em.h splits the work area off.
- Separate local per re-assigned object (r100 R100Init cop[0]/cop[1]): `o = SetObjSmd(); ...; o =
  SetObjSmd();` gives the first `addi r4, o, 0x1d8` an extra anti-dependence (the later re-assignment)
  -> one more dependent -> rank_for_schedule issues it before the argument `li`s (the original has it
  last). `cObj* o; cObj* o2;` — one local per object — drops the edge.
- `RsfSet(G_ROOM_ID, n); W->strId = SndStrReq(a, b, 0x80000003, 0, 0, 0.0f);` (execShowView in r104,
  r108, r118, r11d): the original loads the 0.0 AFTER the RsfSet store; a pool constant is
  RTX_UNCHANGING_P and floats above stores. `static inline f32 FCRef(const f32& v) { return v; }` +
  `static const f32 vol = 0.0f;` read as `FCRef(vol)` stays below (the cSceObj idiom); the `static const`
  takes the pool word's place in `.rodata` (sizes unchanged). Fixed 4 functions.
- `flag = RsfCheck(..); zero = 0; if (flag == 0)` (int locals) puts the else arm's `li 0` above the branch
  (#5 interblock shape, r100). Block-local `cEm* em = W->em;` for the xFC..xFF byte stores keeps the
  original's store order.
- r103 openShelf_main (4 words): two `lis` high halves get r9/r11 swapped. local-alloc orders qtys by
  n_refs/lifetime and, with sched2 on, extends each qty's life by one insn each side (`fake_birth/
  fake_death`), so hard regs of qtys dying just before/born just after are avoided — the two adjacent `lis`
  pseudos take the other's register when their births are one insn apart. Direct constants/FSet forms give
  the same result. OPEN.
- r103 checkCloseCover (4 words, constant-load pairing / `mr r30` position): all 24 declaration orders of
  `w, h, x, z` and the literal form tried; `wzxh` gives 2 words but changes the `.rodata` pool order. OPEN.
- r103 execOpenCover, r11c closeGate: #9 (our jump.c duplicates the rotated loop's exit test; `for`/`>=`
  forms give `cror`/`bns`, `while (!(x < c))` still duplicates, a goto form cross-jumps the peeled
  subtraction).
- r104 execEvent00 (1 word): `if (f & 0x40) skip = 1; if (!(f & 0x40)) {...}` — the target's second test
  is thread_jumped and its compare cse-deleted but the jump stays `bne` (cc0-equivalent compare); ours
  folds it to `b` (cse's record_jump_equiv/qty_comparison_code on the fallthrough path). `if (skip == 0)`
  (+16 bytes) and `== 0` forms tried. OPEN.
- r11d: the openDoor limit is -1.692f, not -1.69f (`.rodata` word 0x12c; check pool constants against
  rodump when a "pool order" diff is a single word). execHide_main (6 words): the target loads `spd = 0.0`
  between `li r4` and `li r6` and issues `li r3,6` last; init-after-call and a `Vec* pos` local do not
  move it. OPEN.
- r118 ThunderMove (2 words, `ori r30`/`li r28` order): declaring the zero inside the EstSet arm makes it
  worse (r31 stores, +0x194). OPEN.
- r117 EventChandelier (-4 bytes): after the first swing loop the target re-materialises `lis r29, pPL@ha`
  into a fresh callee-saved register, ours reuses the loop pseudo (gcse/cse path class, see the cse1 path
  items above). OPEN.
- Header additions: `include/sce_at.h` / `src/game/sce_at.cpp` `SceAtCreateExecAt(cModel* m, Vec* pos,
  int a, int b, int c, f32 h, int d, f32 ang, f32 range, int e, int prio, TaskFunc func, int arg, u8 flag)`
  (parameter order from r117/r11c call sites); `include/obj.h` `class cObjWep* allow;` at 0x200 (line 512).
- mcmp noise reminder: `MISSING cUnit_dt_cUnit/beginEvent/endEvent/op_delete` + `extra _._5cUnit ...` and a
  `.rodata reloc cUnit_dt_cUnit vs _._5cUnit` are naming-only; the REL shasum is the judge.

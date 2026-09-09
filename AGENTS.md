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

`dummy` (stdio syscall stubs), `tealeaf` (`__cvt_fp2unsigned`, `__va_arg`, `__div2i`-style aliases
that `b` to libgcc) and `FSasync` are GCC 2.95 -O2 like the game (`stwu -8; mflr; stw r0,0xc`, `stmw`,
`first.183` statics, r9/r11 temporaries) but with **no small data** (`LIBSN_UNITS` in configure.py:
`cflags_game + -G 0`, `strip_unused.py --gcc`). `proview`, `ppcdown`, `fileserver` (`addi r31,r4,0`
copies, `lis rX,sym@h; nop`), `eabi` (`_savefpr_14/_restfpr_14`) and `__start` (`.init`) are hand-written
assembly; `crt0` is only the data half of that assembly (two 32-byte message buffers, the libsn
version words, `LinkFiddle = {__mod2i, 0}`); `crtbegin` is `.ctor/.dtor` `-1` sentinels;
`builtin-delete` is SN's libstdc++ `operator new/delete` warning unit (C++, everything stripped but
the `bad_alloc` type-info name and the four warning strings). The split object's `.data` alignment
(dtk reports 2**3) is what the DOL layout needs: a GCC object with a 4-aligned `.data` shifts every
following `.data` unit by 4 (dummy: `asm(".section .data\n\t.balign 8\n\t.section .text")`).

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
  `build/G4BE08/<mod>/obj/<mod>/em10.o` is the same in every module) followed by two per-enemy
  objects: `<mod>_prolog.cpp` (`_prolog` = `OSReport("em10 prolog Ok\n")` in every module, stores
  EmXXInit/EmXXSet into EmInitFunc/.data+0; `_epilog`, `_unresolved`; 0x54 bytes) and `<mod>_set.cpp`
  (EmXXInit/EmXXSet/EmXXWeaponSet, 0x780..0xA08 bytes; em1d/em1e/em1f/em20 also include light.h:
  cManager<cLight> code the .sym skips and an unreferenced `"D:/Bio4/Prog/light.h"` string). The
  real names of the two small files are not in the binary. The 28 other enemies (em18, em21..em3d,
  em3e; .text 0x10C8..0x17034) start with `_prolog`/`_epilog`/`_unresolved` (same code, 4 `_prolog`
  variants), then their own `"D:/Bio4/Prog/emXX.cpp"`, and share only cUnit's inline
  `beginEvent`/`endEvent`/`~cUnit`/`operator delete` (byte-identical, at the end).
- Compiler flags: `cflags_game` + `-G 0` (no small data in RELs: every DOL global goes through
  `lis/addi`). Post-build: `fold_linkonce.py --module <mod>` (see "Multi-object modules" below); no strip_unused (nothing is dead-stripped in a -r link).
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
    `Em10Init`), the original linker did not; make_rel writes A back into every global field.
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
- OPEN (rooms): r10d.cpp's .rodata is `[flag_rsf.h][r10d.cpp][HALT %s(%d)]` while every room that also
  includes atari.h has `[..][HALT][flag_rsf.h][rNNN.cpp]`; include/flag_rsf.h gives `[HALT][flag_rsf.h]`
  first. The code of R10dInit/Main matches (src/st1/r10d.cpp).

### Open

- t_emlist.cpp (0x6174 of code: a 0x3E0 work block behind a struct-member pointer reloaded after every
  store, 122 `const char*` name tables and a
  64-entry `{char name[16]; const char** flag, *type, *set, *x}` id table, TOOL_MENU-like char[]
  menus) has 36 of 53 functions matched (skeleton, data and menus done; the disp/camera/target functions
  are left); the stage rooms are split (config/G4BE08/modules.py) but only r10d.cpp's code is written;
  em_wrap.cpp matches in st1_0/st2_4 (Matching) and is one register-allocation diff away elsewhere.
- The `.drs` archives are not rebuilt by `ninja` (`tools/drs.py rebuild` does one at a time); the
  sound bank's record types 1/2 and the p0/p1 parameters are not interpreted.
- Unit boundaries inside the big modules (Tools has ~39 source files) are not known; the `.gnu.linkonce`
  orphan sections of Sscrn/t_esp/t_event/t_id/t_movie/Tools/t_sce (their original ELFs had 12 extra
  sections between .text and .ctors, concatenated behind .text in the REL).

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

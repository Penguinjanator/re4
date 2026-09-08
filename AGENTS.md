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

## Don'ts

- Never run `git stash`, `git checkout -- <file>`, `git reset` or anything else that rewrites the shared
  working tree: other agents are editing it at the same time.

- Never edit `build/`, `build.ninja`, `objdiff.json`, or `config/G4BE08/splits.txt` by hand.
- Do not run interactive `objdiff-cli diff`; use `tools/fdiff.py` (one-shot).
- Do not commit; the orchestrator commits.

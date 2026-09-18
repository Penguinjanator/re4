# Disc 2: the island stage modules st3_0..st3_3 (2026-09-18)

The Nov 25 2004 debug build is two discs. The `main.dol` and every REL on disc 2 are byte-identical
to disc 1's (checked with the disc 2 dump that surfaced in 2023 alongside disc 1: SHA-1 pairing in the
archive), except the four island-stage room modules `files/Rel/st3_0.rel .. st3_3.rel` with their
`Bio4.st3_N.sym`, which only exist on disc 2 (the island chapters are disc 2 content). Disc 2 also
carries `Bio4.em06/em09/emmark/pl03/pl10/pl12/wep03/wep46/st0.sym` without a matching REL (pl03/10/12
archives have no REL, wep03/wep46 are byte copies of wep02/wep13, the others have no archive).

Setup: `config.yml` modules, `tools/gen_rel_config.py` (regenerates every module; only the four new
directories were kept), hashes in `build.sha1` (111 -> 115 files), `tools/extract_orig.py` skips RELs
the given image lacks and `configure.py` extracts from every image in `orig/G4BE08/`. All four RELs
rebuilt byte-identical from their split objects before any source existed: no new `make_rel` cases.

Layout (config/G4BE08/modules.py UNITS): `em_wrap.cpp` — a THIRD revision (`EM_WRAP_ROUTE`:
`cEmRouteRun`/`cEmRouteExec` added to src/st/em_wrap.cpp; st3_3 has the first revision), `cSceObj.cpp`
in all four, one object per room (`r300 r301 r303 r304 r305 r306 r307 r308 r309 r30a r30b r30c r30e`,
`r30d r30f r310 r311 r312`, `r315 r316 r317 r318 r31a r31b r31c r31d`, `r320 r321 r325 r326 r327 r328
r329 r330 r331 r332 r333`), then `st3.cpp` (src/st3/st3.cpp: the St3 room table for all four modules,
plus the count-down timer functions that survive only in st3_3 — st3_0/1 drop the `CountDown::
checkState` linkonce copy, st3_2 keeps it nameless: `LINKONCE_DROP` per module). 49 units, 872
functions, 374 KB of code; all IDENTICAL after 21 agent passes in ~11 hours.

Every room object's `.data` is 8-aligned in the REL: a unit whose real `.data` size is not a multiple
of 8 needs the next unit's `asm(".section .data\n\t.balign 8\n\t.text")` (r320 -> r327, r330 -> r332,
r30b before st3.cpp's linker word; also `.rodata` in r317). Symbols the `.sym` marks `scope:global`
must be non-static even when only one room uses them (`r31b_plParts`, `r332_satPos`, `r303_doorPos`),
or the REL's ADDR16 fields differ (the object matches, the REL does not: `make_rel --verify`).

## Levers found in this pass (beyond the catalogue)

GCC 2.95 mechanisms met for the first time or in a new form; all pure C unless noted.

- **LOOP_END-blinded dead test as a cse1 block splitter** (r307 checkPiece, r318 LaserCallBackFunc,
  r318 AutoDoor, r31b): `int k = 2; do { } while (0); ... if (k != 2) X;` — cse1 (`after_loop = 0`)
  ends its ebb at the LOOP_END note so it cannot fold `k != 2`; with `-fcse-skip-blocks` it follows the
  jump around the arm and `invalidate_skipped_block` kills the classes the arm sets, so a later `(plus
  sym mult)` is not operand-swapped (`fold_rtx` "constant second" needs cse to KNOW the register is a
  constant) and a `(mem P)` is not rewritten to `(mem (plus p K))`. cse2 (`after_loop = 1`) knows `k`,
  folds the jump, skips the arm; jump/flow delete everything before sched1. The surviving loop notes are
  a haifa barrier: place them so the next insn is one that could not move anyway. As a dead `return`
  before an infinite loop it gives the preheader an exit edge at gcse time, so PRE does not hoist the
  loop's single-use highs (block-based lcm kills anticipation only at the last block).
- **cse2-only merges** are not stopped by LOOP_END (`cse_end_of_basic_block` ignores it when
  `after_loop`); a dead SECOND SET of the pseudo in another arm (`zero = em;`, deleted by flow) extends
  its last uid beyond the ebb so `make_regs_eqv` keeps it out of the class (r31b Room03U3Main).
- **A zero that must not be a loop movable** (r30d): `scan_loop` admits a `(set reg const)` only if
  `reg_in_basic_block_p` or not `maybe_never`; a zero set in one block and stored in another is never a
  movable (not `combine_movables` — every condition there holds for two zeros), survives to cse2 and
  joins the reversed biv's class via `record_jump_equiv`, so the store uses the counter register.
- **`x < 25` vs `x <= 24`**: fold-const rewrites a literal compare; the target compared against a
  register: `int m = 25;` (REG_EQUIV, reload puts 25 back, code stays LT) (r31b FallRoom).
- **`volatile` for a sched1 output dependence** (r30b R30bInit, tagged): two stores through the same
  pointer at different offsets are always disambiguated, but `write_dependence_p` returns 1 when both
  MEMs are `MEM_VOLATILE_P`: `volatile cObj* vc = cable; vc->be_flag = ..; vc->pos.y = ..;` gives the
  first store a cost-1 dependent so it ties the pool load's priority and wins on register weight.
- **The r40f "second word pair"** (two `Vec` template copies, L0,L8,L4 vs L0,L4,L8): closed by the
  consumer, not the copies — the following call's object through a struct-member view (`pPLS->setPos`)
  is a mem/s load that alias.c orders after the frame stores (r31b Main/EventS00, r300 R300_Event).
- **Non-static brace initialisers** (`int cnt[5] = {cntB, 5, 5, 8, 15}` with one non-constant element)
  emit element stores instead of a .rodata template + copy, and `store_constructor`'s `(clobber
  (mem:BLK))` keeps a dead element-0 store alive that a memset/template form would DSE (r318).
- **Module-level facts**: a `bl` argument for a variadic callee needs the real extra argument; the
  `.sym` scope decides `static` (r310 `Evt_R310S00_Func` local -> REL field S+A).
- **Hard-register copy chains for `lwzu`** (r318 LaserCallBackFunc, pins): `register cModel* q
  asm("r28"); register Vec* pa asm("r10"); q = p2; pa = q; asm("" : "+r"(q)); pa += 0x70;` — hard regs
  are never canonicalised by cse, the launder makes combine keep `pa = q`, combine forms the update
  with op0 = op1; the loop notes attached to the no-op `q = p2` (deleted after allocation) act as a
  sched1 barrier and vanish before sched2.
- `IntSet(work->hardMode, 0)` — a reference store the following `pG` load depends on, so the store
  outranks a `lis` and the PRE'd high fills the bubble (r317 R317Init, r327 R327Init).
- A single-use `int hp = 0;` stored later is a free sched1 filler in block 0 (update_equiv_regs moves
  the init to the store; bytes identical to the literal) that shifts the hoisted `addi rX,r1,N`
  pseudos one slot (r31b R31bInit).
- `Vec pos0 = {0.0f, 0.0f, 0.0f}` (the `clear_storage` libcall) keeps `&pos0` in one pseudo across
  the block where the varargs `memset` view did not (r317 Elevator2Init).

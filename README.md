Resident Evil 4 (GameCube) decompilation
=========================================

A work-in-progress matching decompilation of *Resident Evil 4* for the Nintendo GameCube,
targeting the `G4BE08` **debug build (Disc 1)**, whose `Bio4.sym` symbol files give every
function's name, size and source object file.

The game was compiled with **SN Systems ProDG** (GCC 2.95.x), not CodeWarrior; the Nintendo SDK
libraries linked into it were prebuilt with CodeWarrior. The build reproduces `main.dol`
byte-for-byte from the start (split objects are linked with the original layout); progress is the
share of that code replaced by compiled source.

This repository does **not** contain any game assets or code from the disc. You must provide your own
disc image.

Building
--------

- Python 3 and [ninja](https://ninja-build.org/) on `PATH`.
- Place the debug Disc 1 image (ISO/GCM) in `orig/G4BE08/`, then create the working DOL with the
  shared `.ctors/.dtors` section split (needed by decomp-toolkit):

  ```sh
  build/tools/dtk vfs cp "orig/G4BE08/<image>.iso:sys/main.dol" orig/G4BE08/sys/main.dol
  build/tools/dtk vfs cp "orig/G4BE08/<image>.iso:files/Bio4.sym" orig/G4BE08/files/Bio4.sym
  python3 tools/dol_sections.py split orig/G4BE08/sys/main.dol orig/G4BE08/sys/main_split.dol 0x8021C9E0
  ```

  (`build/tools/dtk` is downloaded by the first `ninja` run; run `python3 configure.py && ninja` once
  first if you don't have it yet.)

- `python3 configure.py && ninja`

Compilers (ProDG and CodeWarrior) and tools (decomp-toolkit, objdiff, wibo) are downloaded
automatically. `ninja` prints `main.dol: OK` when the linked output matches.
The ProDG compiler proper (`cc1plus`/`cc1`) is a native Linux build of SN Systems' GPL source drop
(GCC 2.95.3 "SN BUILD v1.79"), built by `tools/sn-gcc/build.sh` with two patches: `linux-host.patch`
(host config) and `shipped-build-temp-flags.patch` (a reconstructed behaviour of the compiler build the
game shipped with; see the patch header and AGENTS.md "Compiler").

Layout
------

- `config/G4BE08/` — `symbols.txt`, `splits.txt`, `objects.py` (unit list + matching status),
  `ldscript.ld`, `sym_map.tsv` (address → original demangled name). Generated from `Bio4.sym` by
  `tools/gen_config.py`; symbol names are updated to the compiler's mangled names by
  `tools/sync_symbols.py` as units are matched.
- `src/game/` — game code (C++, and newlib C units), `src/lib/` — SDK and runtime.
- `include/` — headers.
- `tools/` — build generator (`project.py`), `unit_info.py`, `fdiff.py`, `sync_symbols.py`.
- `AGENTS.md` — the matching workflow.

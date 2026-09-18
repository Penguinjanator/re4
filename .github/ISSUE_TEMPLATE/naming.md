---
name: Struct or field name
about: Propose a name for a placeholder field (xNN, pad_NN), a struct, an enum or a function parameter
title: "name: <Struct>::<field> -> <proposed>"
labels: naming
---

Read `docs/naming.md` (the three classes of field names) and `CONTRIBUTING.md`, "Proposing a rename",
first.

**Struct and header**
<!-- e.g. Em10Work, include/em10.h -->

**Field (current name, offset, type)**
<!-- e.g. x24, 0x024 (0x404 in cEm), Vec -->

**Proposed name**
<!-- follows the spelling of the surrounding struct -->

**Evidence class**
<!-- one of: PS2 debug symbols / usage / other vendor artefact -->

**Evidence**
<!--
PS2: paste the row from `python3 tools/ps2sym.py --struct <Name>` (GC field, PS2 field, confidence, reasons).
Usage: the functions that read and write the field (file and function name, not line numbers), what they do
with it, and why that fixes the meaning.
Other: quote the string / spec / debug-menu label.
-->

**Other fields this affects**
<!-- neighbours whose alignment changes, or the same field in another struct -->

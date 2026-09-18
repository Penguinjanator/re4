---
name: Build or verify failure
about: configure, ninja, dtk shasum, bytecmp or asmcheck does not behave as documented
title: "build: <one line>"
labels: bug
---

**Commit**
<!-- `git rev-parse HEAD` -->

**Command**
<!-- the exact command line, from the repository root -->

**Output**
<!-- paste the failing output; for `dtk shasum` the FAILED lines, for `bytecmp.py` the per-function list -->

**Environment**
<!-- distribution, Python version, how the native SN GCC was built (tools/sn-gcc/build.sh, SN source drop version),
which disc images are in orig/G4BE08/ (disc 1, disc 2, both) -->

**Clean rebuild**
<!-- did you run the clean rebuild from CONTRIBUTING.md, "Verify", step 1? paste its shasum summary -->

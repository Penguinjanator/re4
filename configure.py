#!/usr/bin/env python3

###
# Generates build files for the project.
# This file also includes the project configuration,
# such as compiler flags and the object matching status.
#
# Usage:
#   python3 configure.py
#   ninja
#
# Append --help to see available options.
###

import argparse
import importlib.util
import re
import sys
from pathlib import Path
from typing import Any, Dict, List

from tools.project import (
    Object,
    ProgressCategory,
    ProjectConfig,
    calculate_progress,
    generate_build,
    is_windows,
)

# Game versions
DEFAULT_VERSION = 0
VERSIONS = [
    "G4BE08",  # 0: Resident Evil 4 (USA) Debug Disc 1
]

parser = argparse.ArgumentParser()
parser.add_argument(
    "mode",
    choices=["configure", "progress"],
    default="configure",
    help="script mode (default: configure)",
    nargs="?",
)
parser.add_argument(
    "-v",
    "--version",
    choices=VERSIONS,
    type=str.upper,
    default=VERSIONS[DEFAULT_VERSION],
    help="version to build",
)
parser.add_argument(
    "--build-dir",
    metavar="DIR",
    type=Path,
    default=Path("build"),
    help="base build directory (default: build)",
)
parser.add_argument(
    "--binutils",
    metavar="BINARY",
    type=Path,
    help="path to binutils (optional)",
)
parser.add_argument(
    "--compilers",
    metavar="DIR",
    type=Path,
    help="path to compilers (optional)",
)
parser.add_argument(
    "--map",
    action="store_true",
    help="generate map file(s)",
)
parser.add_argument(
    "--debug",
    action="store_true",
    help="build with debug info (non-matching)",
)
if not is_windows():
    parser.add_argument(
        "--wrapper",
        metavar="BINARY",
        type=Path,
        help="path to wibo or wine (optional)",
    )
parser.add_argument(
    "--dtk",
    metavar="BINARY | DIR",
    type=Path,
    help="path to decomp-toolkit binary or source (optional)",
)
parser.add_argument(
    "--objdiff",
    metavar="BINARY | DIR",
    type=Path,
    help="path to objdiff-cli binary or source (optional)",
)
parser.add_argument(
    "--sjiswrap",
    metavar="EXE",
    type=Path,
    help="path to sjiswrap.exe (optional)",
)
parser.add_argument(
    "--ninja",
    metavar="BINARY",
    type=Path,
    help="path to ninja binary (optional)",
)
parser.add_argument(
    "--verbose",
    action="store_true",
    help="print verbose output",
)
parser.add_argument(
    "--non-matching",
    dest="non_matching",
    action="store_true",
    help="builds equivalent (but non-matching) or modded objects",
)
parser.add_argument(
    "--warn",
    dest="warn",
    type=str,
    choices=["all", "off", "error"],
    help="how to handle warnings",
)
parser.add_argument(
    "--no-progress",
    dest="progress",
    action="store_false",
    help="disable progress calculation",
)
args = parser.parse_args()

config = ProjectConfig()
config.version = str(args.version)
version_num = VERSIONS.index(config.version)

# Apply arguments
config.build_dir = args.build_dir
config.dtk_path = args.dtk
config.objdiff_path = args.objdiff
config.binutils_path = args.binutils
config.compilers_path = args.compilers
config.generate_map = args.map
config.non_matching = args.non_matching
config.sjiswrap_path = args.sjiswrap
config.ninja_path = args.ninja
config.progress = args.progress
if not is_windows():
    config.wrapper = args.wrapper
# Don't build asm unless we're --non-matching
if not config.non_matching:
    config.asm_dir = None

# Tool versions
config.binutils_tag = "2.42-1"
config.compilers_tag = "20251118"
config.dtk_tag = "v1.8.3"
config.objdiff_tag = "v3.6.1"
config.sjiswrap_tag = "v1.2.2"
config.wibo_tag = "1.0.3"

# Project
config.config_path = Path("config") / config.version / "config.yml"
config.check_sha_path = Path("config") / config.version / "build.sha1"
config.asflags = [
    "-mgekko",
    "--strip-local-absolute",
    "-I include",
    f"-I build/{config.version}/include",
    f"--defsym BUILD_VERSION={version_num}",
]
# ProDG ngcld does not accept Metrowerks linker flags.
config.ldflags = []

# Use for any additional files that should cause a re-configure when modified
config.reconfig_deps = [Path("config") / config.version / "objects.py"]

# Optional numeric ID for decomp.me preset
config.scratch_preset_id = None

# The game was built with SN Systems ProDG (GCC 2.95.x). Version to be confirmed
# once the first game function is matched; 3.9.3 (gcc 2.95.3) is the newest.
PRODG_VERSION = "ProDG/3.9.3"
config.linker_version = PRODG_VERSION
config.prodg_ldscript = Path("config") / config.version / "ldscript.ld"
config.prodg_sda_base = 0x8031BEE0
config.prodg_sda2_base = 0x8032BEE0
config.prodg_stack_end = 0x80316CE0  # _stack_end; _stack_addr (stack top) is 0x8031ACE0 (see ldscript.ld)
# .init has no symbols in Bio4.sym; __start is defined in the linker script
config.entry_override = "__start"

project_root = Path(__file__).resolve().parent
include_dirs = [
    project_root / "include",
    project_root / "src",
    project_root / "build" / config.version / "include",
]

# Base flags for game code (ProDG / GCC 2.95).
# -mfast-cast (not -mps-float): the original saves callee-saved FPRs in 8 bytes each and converts
# s8/u8 to float through a stack byte + psq_l (qr2/qr4). -mps-float reserves 16 bytes per saved FPR,
# which does not match any function in game/cam_ctrl that saves f31.
cflags_game = [
    "-O2",
    "-mfast-cast",
    *[f"-I {d.as_posix()}" for d in include_dirs],
    f"-DBUILD_VERSION={version_num}",
    f"-DVERSION_{config.version}",
]
if args.debug:
    cflags_game.extend(["-g2", "-DDEBUG=1"])
else:
    cflags_game.append("-DNDEBUG=1")

# Nintendo Dolphin SDK libraries were prebuilt by Nintendo with CodeWarrior. The DOL carries
# "<< Dolphin SDK - OS release build: May 21 2004 (0x2301) >>" (OS = SDK 2004 patch 1, i.e.
# SDK_REVISION 1; the other libs are the Apr 5-7 2004 base builds, identical for revision 0/1).
# Sources come from doldecomp/dolsdk2004 (src/lib/<Name>.c, headers in include/dolphin/).
MWCC_SDK_VERSION = "GC/1.2.5n"
cflags_mw_sdk = [
    "-nodefaults",
    "-proc gekko",
    "-fp hard",
    "-Cpp_exceptions off",
    "-enum int",
    "-char unsigned",
    "-warn pragmas",
    "-requireprotos",
    "-pragma 'cats off'",
    "-O4,p",
    "-inline auto",
    "-I-",
    f"-i {(project_root / 'include').as_posix()}",
    f"-i {(project_root / 'include' / 'libc').as_posix()}",
    f"-i {(project_root / 'src' / 'lib').as_posix()}",
    "-D__GEKKO__",
    "-DSDK_REVISION=1",
]

# SDK objects, grouped like the SDK archives they came from (os.a, gx.a, ...).
# Runtime objects compiled with ProDG (libgcc, crt, SN debugger stub, __start,
# __ppc_eabi_init, tors) are NOT listed here and stay in the ProDG "lib" library.
SDK_LIBS: Dict[str, List[str]] = {
    "os": [
        "OS", "OSAlarm", "OSAlloc", "OSArena", "OSAudioSystem", "OSCache", "OSContext",
        "OSError", "OSExec", "OSFatal", "OSFont", "OSInterrupt", "OSLink", "OSMemory",
        "OSMutex", "OSReboot", "OSReset", "OSResetSW", "OSRtc", "OSSemaphore",
        "OSStopwatch", "OSSync", "OSThread", "OSTime",
    ],
    "base": ["PPCArch"],
    "exi": ["EXIBios", "EXIUart"],
    "si": ["SIBios", "SISamplingRate"],
    "db": ["db"],
    "mtx": ["mtx", "mtxvec", "mtx44", "mtx44vec", "vec", "quat", "psmtx"],
    "dvd": ["dvdlow", "dvdfs", "dvd", "dvdqueue", "dvderror", "dvdidutils", "dvdFatal", "fstload"],
    "vi": ["vi"],
    "pad": ["Pad", "Padclamp"],
    "ai": ["ai"],
    "ar": ["ar", "arq"],
    "ax": ["AX", "AXAlloc", "AXAux", "AXCL", "AXOut", "AXSPB", "AXVPB", "AXProf", "AXComp", "DSPCode"],
    "axfx": ["axfx", "reverb_hi", "reverb_std", "chorus", "delay", "reverb_hi_4ch"],
    "mix": ["mix"],
    "axart": ["axart", "axartsound", "axartcents", "axartenv", "axartlfo", "axart3d", "axartlpf"],
    "syn": ["syn", "synctrl", "synenv", "synlfo", "synmix", "synpitch", "synsample", "synvoice", "synwt"],
    "seq": ["seq"],
    "dsp": ["dsp", "dsp_debug", "dsp_task"],
    "card": [
        "CARDBios", "CARDBlock", "CARDDir", "CARDCheck", "CARDMount", "CARDFormat", "CARDOpen",
        "CARDCreate", "CARDRead", "CARDWrite", "CARDDelete", "CARDStat", "CARDNet", "CARDUnlock",
        "CARDRdwr",
    ],
    "gx": [
        "GXAttr", "GXGeometry", "GXLight", "GXTexture", "GXBump", "GXTev", "GXPixel", "GXTransform",
        "GXInit", "GXFifo", "GXMisc", "GXFrameBuf", "GXPerf", "GXDraw", "GXDisplayList",
    ],
    "texPalette": ["texPalette"],
    "fileCache": ["fileCache"],
    "amcstubs": ["AmcExi2Stubs"],
    "odemustubs": ["DebuggerDriver"],
}
SDK_UNIT_LIB: Dict[str, str] = {
    f"lib/{name}.c": lib for lib, names in SDK_LIBS.items() for name in names
}
# Per-object deviations from cflags_mw_sdk (same as in doldecomp/dolsdk2004's Makefile). These
# replace the base flag: MWCC keeps the first -O level it sees, so appending "-O3,p" has no effect.
SDK_CFLAG_OVERRIDES: Dict[str, Dict[str, str]] = {
    **{f"lib/{name}.c": {"-char unsigned": "-char signed"} for name in SDK_LIBS["dvd"]},
    "lib/mtx.c": {"-char unsigned": "-char signed"},
    "lib/mtx44.c": {"-char unsigned": "-char signed"},
    "lib/CARDOpen.c": {"-char unsigned": "-char signed"},
    "lib/EXIBios.c": {"-O4,p": "-O3,p"},
}


def sdk_cflags(unit: str) -> List[str]:
    repl = SDK_CFLAG_OVERRIDES.get(unit)
    if not repl:
        return cflags_mw_sdk
    return [repl.get(flag, flag) for flag in cflags_mw_sdk]


def DolphinLib(lib_name: str, objects: List[Object]) -> Dict[str, Any]:
    return {
        "lib": lib_name,
        "mw_version": MWCC_SDK_VERSION,
        "cflags": cflags_mw_sdk,
        "mwcc_depflag": "-MD",  # with -I- the default -MMD records no header dependencies
        "progress_category": "sdk",
        "objects": objects,
    }

Matching = True                   # Object matches and should be linked
NonMatching = False               # Object does not match and should not be linked
Equivalent = config.non_matching  # Object should be linked when configured with --non-matching


def MatchingFor(*versions):
    return config.version in versions


# Unit list generated from the debug build's Bio4.sym (see tools/gen_config.py).
spec = importlib.util.spec_from_file_location("objects", Path("config") / config.version / "objects.py")
objects_mod = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(objects_mod)
UNITS: List[str] = objects_mod.UNITS
MATCHING: Dict[str, bool] = getattr(objects_mod, "MATCHING", {})

# Split unit names come from splits.txt (game/ units are all named *.cpp there). A UNITS entry
# may instead name a *.c source (newlib units): the Object keeps the split name, the source is the .c.
split_units = set()
with open(Path("config") / config.version / "splits.txt") as _f:
    for _line in _f:
        _m = re.match(r"^(\S+):\s*$", _line)
        if _m:
            split_units.add(_m.group(1))

game_objects: List[Object] = []
lib_objects: List[Object] = []
sdk_objects: Dict[str, List[Object]] = {lib: [] for lib in SDK_LIBS}
for unit in UNITS:
    status = MATCHING.get(unit, NonMatching)
    name = unit
    if unit not in split_units:
        alt = str(Path(unit).with_suffix(".cpp"))
        if alt in split_units:
            name = alt
    if unit.startswith("game/"):
        # GCC 2.95 linkonce sections (vtables, template instantiations, out-of-line inlines) are
        # folded into .rodata / dropped the way the original link laid them out (see the tool).
        # The newlib C units (game/*.c) were built with -ffunction-sections: functions nobody
        # referenced were dropped by the linker while their .rodata strings stayed; strip them too.
        post_build = [f"$python tools/fold_linkonce.py --unit {unit} {{out}}"]
        post_build_implicit = [Path("tools/fold_linkonce.py"), Path("config") / config.version / "sym_map.tsv"]
        if unit.endswith(".c"):
            post_build.insert(0, f"$python tools/strip_unused.py --unit {name} {{out}}")
            post_build_implicit.append(Path("tools/strip_unused.py"))
        game_objects.append(
            Object(
                status,
                name,
                source=unit,
                post_build=post_build,
                post_build_implicit=post_build_implicit,
            )
        )
    elif unit in SDK_UNIT_LIB:
        # The ProDG linker dead-stripped unreferenced SDK functions; drop them from our object too.
        sdk_objects[SDK_UNIT_LIB[unit]].append(
            Object(
                status,
                name,
                source=unit,
                cflags=sdk_cflags(unit),
                post_build=[f"$python tools/strip_unused.py --unit {unit} {{out}}"],
                post_build_implicit=[Path("tools/strip_unused.py"), Path("config") / config.version / "sym_map.tsv"],
            )
        )
    else:
        lib_objects.append(Object(status, name, source=unit))

config.warn_missing_config = True
config.warn_missing_source = False
config.libs = [
    {
        "lib": "Bio4",
        "mw_version": PRODG_VERSION,
        "cflags": cflags_game,
        "progress_category": "game",
        "objects": game_objects,
    },
    {
        # ProDG-compiled runtime: libgcc (_ashldi3, _divdi3, _eh, ...), crt (__start,
        # __ppc_eabi_init, __main, tors, crtbegin), SN debugger stub (ppcdown, fileserver,
        # proview, tealeaf), newlib libm, and third-party libs (CRI ADX/Sofdec) not yet identified.
        "lib": "lib",
        "mw_version": PRODG_VERSION,
        "cflags": cflags_game,
        "progress_category": "sdk",
        "objects": lib_objects,
    },
    *[DolphinLib(lib, objs) for lib, objs in sdk_objects.items() if objs],
]

config.progress_categories = [
    ProgressCategory("game", "Game Code"),
    ProgressCategory("sdk", "SDK/Runtime Code"),
]
config.progress_each_module = args.verbose
config.progress_report_args = []

if args.mode == "configure":
    # Write build.ninja and objdiff.json
    generate_build(config)
elif args.mode == "progress":
    # Print progress information
    calculate_progress(config)
else:
    sys.exit("Unknown mode: " + args.mode)

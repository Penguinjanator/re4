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
config.prodg_stack_end = 0x8031ACE0
# .init has no symbols in Bio4.sym; __start is defined in the linker script
config.entry_override = "__start"

project_root = Path(__file__).resolve().parent
include_dirs = [
    project_root / "include",
    project_root / "src",
    project_root / "build" / config.version / "include",
]

# Base flags for game code (ProDG / GCC 2.95). Optimization level to be confirmed.
cflags_game = [
    "-O2",
    "-mps-float",
    *[f"-I {d.as_posix()}" for d in include_dirs],
    f"-DBUILD_VERSION={version_num}",
    f"-DVERSION_{config.version}",
]
if args.debug:
    cflags_game.extend(["-g2", "-DDEBUG=1"])
else:
    cflags_game.append("-DNDEBUG=1")

# Metrowerks cflags for Nintendo SDK libraries (prebuilt by Nintendo with CodeWarrior).
cflags_mw_sdk = [
    "-nodefaults",
    "-proc gekko",
    "-align powerpc",
    "-enum int",
    "-fp hardware",
    "-Cpp_exceptions off",
    "-O4,p",
    "-inline auto",
    "-RTTI off",
    "-fp_contract on",
    "-str reuse",
    *[f"-i {d.as_posix()}" for d in include_dirs],
    f"-DBUILD_VERSION={version_num}",
    f"-DVERSION_{config.version}",
    "-DNDEBUG=1",
]

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

game_objects: List[Object] = []
lib_objects: List[Object] = []
for unit in UNITS:
    status = MATCHING.get(unit, NonMatching)
    if unit.startswith("game/"):
        game_objects.append(Object(status, unit))
    else:
        lib_objects.append(Object(status, unit))

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
        # Nintendo SDK, MSL/libgcc runtime, SN debugger stub. Compiler varies per object;
        # treated as ProDG by default until each library is identified.
        "lib": "lib",
        "mw_version": PRODG_VERSION,
        "cflags": cflags_game,
        "progress_category": "sdk",
        "objects": lib_objects,
    },
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

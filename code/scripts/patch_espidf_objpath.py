"""
patch_espidf_objpath.py

Pre-build PlatformIO extra_script.

Fixes a known object-file-naming collision in PlatformIO's platform-espressif32
builder (builder/frameworks/espidf.py, compile_source_files()). Sources that
are (a) reported with an absolute path by CMake's file API and (b) NOT under
$FRAMEWORK_DIR/components (e.g. anything under managed_components/) get their
object path collapsed to just the basename, discarding directory structure.
Two files with the same name in different subdirectories (e.g.
tensorflow/lite/micro/kernels/circular_buffer.cc and
signal/src/circular_buffer.cc, both in espressif__esp-tflite-micro) then
collide on the same .o path and SCons aborts with:

*** Multiple ways to build the same target were specified for: ...circular_buffer.cc.o

This script patches that fallback branch to keep the last few path segments
(e.g. "signal/src/circular_buffer.cc" instead of just "circular_buffer.cc")
rather than trying to prove the source lives under $PROJECT_DIR by string
prefix comparison. Prefix comparison is fragile in practice -- e.g. on
Windows with a VMware/network shared folder, CMake's file API may report an
absolute source path via a UNC path (\\host\share\...) while $PROJECT_DIR
resolves via a mapped drive letter (Z:\...) for the *same* underlying file,
causing a naive prefix check to wrongly report "not under project" and fall
through to the basename-only behavior this patch is meant to fix. Keeping
trailing path segments sidesteps that problem entirely. It is idempotent
and safe to run on every build: if the patch is already applied, it's a
no-op.

NOTE: This patches a file shared by the *platform installation*
(~/.platformio/platforms/espressif32/builder/frameworks/espidf.py), which
is shared across all projects/environments using that installed platform
version on this machine. Re-running `pio pkg update` / reinstalling the
platform will fetch a fresh, unpatched copy -- this script will silently
re-patch it again on the next build, so no manual re-application is needed.
"""

import os
import re

Import("env")

# Matches: obj_path, os.path.basename(src_path)
# (the exact call inside the collapsing `else` branch), tolerant of
# whatever whitespace/line-wrapping the installed version uses.
PATTERN = re.compile(
    r"obj_path,\s*os\.path\.basename\(src_path\)"
)

# Number of trailing path segments to keep (directories + filename).
# 3 is enough to disambiguate e.g. "signal/src/circular_buffer.cc" from
# "tensorflow/lite/micro/kernels/circular_buffer.cc" without relying on
# any prefix comparison against $PROJECT_DIR (which can be fragile across
# drive letters, UNC paths, symlinks, or WSL/network-mounted paths).
TRAILING_SEGMENTS = 3

# IMPORTANT: the replacement is wrapped in its own parentheses and carries
# NO trailing comment. The matched pattern sits *inside* an enclosing
# os.path.join(obj_path, ...) call whose closing ")" is NOT part of the
# match -- appending a "# comment" after the replacement would turn that
# original closing paren into commented-out text and break the statement
# (this was the bug in an earlier version of this patch). The idempotency
# marker below is a plain code string, not a comment, so it can never
# swallow surrounding syntax.
_UNIQUE_MARKER_TOKEN = "Path(src_path).parts[-"  # only ever appears after patching

REPLACEMENT = (
    "obj_path, "
    "(os.path.join(*Path(src_path).parts[-{n}:]) "
    "if len(Path(src_path).parts) >= {n} "
    "else os.path.basename(src_path))"
).format(n=TRAILING_SEGMENTS)


def patch_espidf_objpath():
    platform = env.PioPlatform()
    espidf_py_path = os.path.join(
        platform.get_dir(), "builder", "frameworks", "espidf.py"
    )

    if not os.path.isfile(espidf_py_path):
        print(f"[patch_espidf_objpath] Skipped: {espidf_py_path} not found")
        return

    with open(espidf_py_path, "r", encoding="utf-8") as fp:
        content = fp.read()

    if _UNIQUE_MARKER_TOKEN in content:
        # Already patched, nothing to do.
        return

    new_content, count = PATTERN.subn(REPLACEMENT, content)

    if count == 0:
        print(
            "[patch_espidf_objpath] WARNING: Could not find the target "
            "pattern in espidf.py. The installed platform-espressif32 "
            "version may have changed this function -- patch not applied. "
            "File: " + espidf_py_path
        )
        return

    if count > 1:
        print(
            f"[patch_espidf_objpath] WARNING: Found {count} matches "
            "instead of 1 -- refusing to patch to avoid unintended changes. "
            "File: " + espidf_py_path
        )
        return

    with open(espidf_py_path, "w", encoding="utf-8") as fp:
        fp.write(new_content)

    print(f"[patch_espidf_objpath] Patched: {espidf_py_path}")


patch_espidf_objpath()
"""PlatformIO pre-script: generate main/version.cpp and sd-card/html/version.txt.

Overrides (platformio.ini option / environment variable):
  PIO: custom_version_tag      / ENV: OVERRIDE_GIT_TAG     (string)
  PIO: custom_version_branch   / ENV: OVERRIDE_GIT_BRANCH  (string)
  PIO: custom_version_dirty    / ENV: OVERRIDE_DIRTY       (true|1|+)

Environment:
  STRICT_VERSION=1        fail the build if git metadata can't be determined
                          and no override is given (use in CI)

GitHub pull_request builds check out a detached synthetic merge commit. There
the branch is taken from GITHUB_HEAD_REF and the revision from the PR head SHA
in the event payload (GITHUB_EVENT_PATH), not from the merge commit.

Requires in sdkconfig (IDF's own esp_app_desc is weak; ours replaces it):
  CONFIG_APP_EXCLUDE_PROJECT_VER_VAR=y
  CONFIG_APP_EXCLUDE_PROJECT_NAME_VAR=y
Both generated files (version.cpp, version.txt) should be in .gitignore.
"""

import json
import os
import subprocess
import sys
from datetime import datetime, timezone

Import("env")  # noqa: F821  (provided by PlatformIO/SCons)

PROJECT_NAME = "AI-on-the-Edge-Device"
IDF_VERSION_MAX = 31  # esp_app_desc_t.version is char[32]
SRC_EXT = (".c", ".cpp", ".h", ".hpp")


# --------------------------------------------------------------------------
# Logging
# --------------------------------------------------------------------------
def _truthy(v):
    return str(v).strip().lower() not in ("", "0", "false", "none")


def _verbose():
    try:
        from SCons.Script import ARGUMENTS as scons_args
    except Exception:  # noqa: BLE001
        scons_args = {}
    candidates = [
        scons_args.get("PIOVERBOSE"),
        scons_args.get("VERBOSE"),
        env.get("PIOVERBOSE"),
        env.get("VERBOSE"),
    ]

    if any(a in sys.argv for a in ("-v", "--verbose")):
        return True
    return any(c is not None and _truthy(c) for c in candidates)


def dprint(msg):
    if _verbose():
        sys.stderr.write(f"VERSION INFO: {msg}\n")
        sys.stderr.flush()


def warn(msg):
    sys.stderr.write(f"VERSION INFO: [WARNING] {msg}\n")
    sys.stderr.flush()


# --------------------------------------------------------------------------
# Helpers
# --------------------------------------------------------------------------
def c_escape(s):
    return s.replace("\\", "\\\\").replace('"', '\\"')


def write_if_changed(path, content):
    """Write only when content differs, so mtimes (and rebuilds) stay stable."""
    try:
        with open(path, "r", encoding="utf-8", newline="") as f:
            if f.read() == content:
                return False
    except FileNotFoundError:
        pass
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(content)
    return True


def fit_version(base, suffix, limit=IDF_VERSION_MAX):
    """Truncate `base`, never the suffix (rev / dirty flag)."""
    room = max(limit - len(suffix), 0)
    return (base[:room] + suffix)[:limit]


def norm(p):
    return os.path.normcase(os.path.realpath(p))


def git(project_dir, *args):
    return subprocess.run(
        ["git", *args], cwd=project_dir, capture_output=True, text=True, encoding="utf-8", errors="replace"
    )


def is_dirty(project_dir, ignore_paths):
    """True if tracked files changed or untracked source files exist,
    ignoring the generated files. `git status` refreshes the index, so
    stat-only changes don't give false positives."""
    top = git(project_dir, "rev-parse", "--show-toplevel")
    if top.returncode != 0:
        return False
    toplevel = top.stdout.strip()

    res = git(project_dir, "status", "--porcelain", "-z", "-uall")
    if res.returncode != 0:
        warn(f"git status failed: {res.stderr.strip()}")
        return False

    ignore = {norm(p) for p in ignore_paths}
    parts = res.stdout.split("\0")
    i = 0
    while i < len(parts):
        entry = parts[i]
        i += 1
        if len(entry) < 4:
            continue
        xy, path = entry[:2], entry[3:]
        if "R" in xy or "C" in xy:
            i += 1  # skip the original-path field
        if norm(os.path.join(toplevel, path)) in ignore:
            continue
        if xy == "??":
            if path.lower().endswith(SRC_EXT):
                return True
        else:
            return True
    return False


def apply_github_pr_info(info):
    """On GitHub pull_request runs HEAD is a detached synthetic merge commit.
    Use the PR's source branch and real head SHA instead."""
    if os.environ.get("GITHUB_EVENT_NAME") not in ("pull_request", "pull_request_target"):
        return

    head_ref = os.environ.get("GITHUB_HEAD_REF")
    if head_ref:
        info["branch"] = head_ref

    event_path = os.environ.get("GITHUB_EVENT_PATH")
    if not event_path:
        return
    try:
        with open(event_path, "r", encoding="utf-8") as f:
            sha = json.load(f)["pull_request"]["head"]["sha"]
        info["rev"] = sha[:7]
        info["pr"] = str(payload["pull_request"]["number"])
    except (KeyError, TypeError, OSError, ValueError) as e:
        warn(f"could not read PR head SHA from event payload: {e}")


def collect_git_info(project_dir, ignore_paths):
    info = {"tag": "N/A", "branch": "N/A", "rev": "", "dirty": "", "pr": "", "ok": False}
    try:
        rev = git(project_dir, "rev-parse", "--short=7", "HEAD")
        if rev.returncode != 0 or not rev.stdout.strip():
            warn(f"git rev-parse failed: {rev.stderr.strip() or 'no output'}")
            return info
        info["rev"] = rev.stdout.strip()
        info["ok"] = True

        tag = git(project_dir, "describe", "--exact-match", "--tags")
        if tag.returncode == 0 and tag.stdout.strip():
            info["tag"] = tag.stdout.strip()

        br = git(project_dir, "rev-parse", "--abbrev-ref", "HEAD")
        if br.returncode == 0:
            info["branch"] = br.stdout.strip()
        # Detached HEAD (typical in CI): fall back to CI-provided ref name
        if info["branch"] in ("HEAD", "N/A", ""):
            info["branch"] = (
                os.environ.get("GITHUB_HEAD_REF")            # GitHub PR: source branch
                or os.environ.get("GITHUB_REF_NAME")         # GitHub push/tag
                or os.environ.get("CI_MERGE_REQUEST_SOURCE_BRANCH_NAME")  # GitLab MR
                or os.environ.get("CI_COMMIT_REF_NAME")      # GitLab push/tag
                or "detached"
            )

        # GitHub PR: replace merge-commit branch/rev with the PR's own
        apply_github_pr_info(info)

        if is_dirty(project_dir, ignore_paths):
            info["dirty"] = "+"
    except Exception as e:  # noqa: BLE001
        warn(f"git metadata extraction failed: {e}")
    dprint(f"git info: {info}")
    return info


# --------------------------------------------------------------------------
# version.cpp template (placeholders instead of f-string brace escaping)
# --------------------------------------------------------------------------
CPP_TEMPLATE = """\
// Auto-generated by generate_version.py - do not edit.
#include "version.h"
#include "sdkconfig.h"
#include "esp_app_desc.h"

#ifndef IDF_VER
#define IDF_VER "unknown"
#endif

extern "C" {
extern const esp_app_desc_t esp_app_desc;
}

const char* const GIT_TAG = "@GIT_TAG@";
const char* const GIT_BRANCH = "@GIT_BRANCH@";
const char* const GIT_REV = "@GIT_REV@";
const char* const BUILD_TIME = "@BUILD_TIME@";

// Strong definition overrides IDF's weak one. Must live in .rodata_desc so it
// sits at the fixed offset the bootloader/OTA/esptool expect.
const esp_app_desc_t esp_app_desc __attribute__((section(".rodata_desc"), used)) = {
    .magic_word = ESP_APP_DESC_MAGIC_WORD,
    .secure_version = 0,
    .version = "@IDF_APP_VERSION@",
    .project_name = "@PROJECT_NAME@",
    .time = __TIME__,
    .date = __DATE__,
    .idf_ver = IDF_VER,
    .app_elf_sha256 = {0},
#ifdef CONFIG_ESP_EFUSE_BLOCK_REV_MIN_FULL
    .min_efuse_blk_rev_full = CONFIG_ESP_EFUSE_BLOCK_REV_MIN_FULL,
#endif
#ifdef CONFIG_ESP_EFUSE_BLOCK_REV_MAX_FULL
    .max_efuse_blk_rev_full = CONFIG_ESP_EFUSE_BLOCK_REV_MAX_FULL,
#endif
#ifdef CONFIG_MMU_PAGE_SIZE
    .mmu_page_size = 31 - __builtin_clz(CONFIG_MMU_PAGE_SIZE),
#endif
};
"""


def render_cpp(v):
    out = CPP_TEMPLATE
    for key, val in v.items():
        out = out.replace(f"@{key}@", c_escape(val))
    return out


# --------------------------------------------------------------------------
# Main
# --------------------------------------------------------------------------
def generate_version_header():
    dprint("=== generate_version.py start ===")

    project_dir = env.get("PROJECT_DIR")
    main_dir = os.path.join(project_dir, "main")
    file_version_cpp = os.path.join(main_dir, "version.cpp")
    sd_html_dir = os.path.join(project_dir, "..", "sd-card", "html")
    file_version_txt = os.path.join(sd_html_dir, "version.txt")

    # Overrides
    custom_tag = env.GetProjectOption("custom_version_tag", os.environ.get("OVERRIDE_GIT_TAG", ""))
    custom_branch = env.GetProjectOption("custom_version_branch", os.environ.get("OVERRIDE_GIT_BRANCH", ""))
    force_dirty = str(env.GetProjectOption("custom_version_dirty", os.environ.get("OVERRIDE_DIRTY", "")))

    git_info = collect_git_info(project_dir, [file_version_cpp, file_version_txt])

    git_tag, git_branch = git_info["tag"], git_info["branch"]
    git_rev, git_diff = git_info["rev"], git_info["dirty"]

    if custom_tag:
        git_tag = custom_tag
    if custom_branch:
        git_branch = custom_branch
    if force_dirty.lower() in ("true", "1", "+"):
        git_diff = "+"

    have_overrides = bool(custom_tag and custom_branch)
    if not git_info["ok"] and not have_overrides:
        msg = "git metadata unavailable and no complete override set; version will be 'unknown'"
        if os.environ.get("STRICT_VERSION") == "1":
            sys.stderr.write(f"VERSION INFO: [ERROR] {msg}\n")
            env.Exit(1)
        warn(msg)
    if not git_rev:
        git_rev = "unknown"

    pr = git_info.get("pr", "")
    pr_branch = f"pr{pr}-{git_branch}" if pr else git_branch
    # Version strings
    if git_tag == "N/A":
        version_tag = f"{pr_branch}-{git_rev}{git_diff}"
        idf_app_version = fit_version(pr_branch, f"-{git_rev}{git_diff}")
    else:
        version_tag = f"{git_tag}{git_diff}"
        idf_app_version = fit_version(git_tag, git_diff)

    # Build time
    now = datetime.now(timezone.utc)
    build_time = now.strftime("%Y-%m-%dT%H:%M:%S+0000")

    values = {
        # "N/A" when HEAD isn't on a tag; otherwise the tag (+ dirty flag)
        "GIT_TAG": "N/A" if git_tag == "N/A" else version_tag,
        "GIT_BRANCH": git_branch,
        "GIT_REV": f"{git_rev}{git_diff}",
        "BUILD_TIME": build_time,
        "IDF_APP_VERSION": idf_app_version,
        "PROJECT_NAME": PROJECT_NAME,
    }

    try:
        changed = write_if_changed(file_version_cpp, render_cpp(values))
        dprint(f"version.cpp {'written' if changed else 'unchanged'}")
    except OSError as e:
        warn(f"could not write {file_version_cpp}: {e}")
        if os.environ.get("STRICT_VERSION") == "1":
            env.Exit(1)

    try:
        changed = write_if_changed(file_version_txt, version_tag)
        dprint(f"version.txt {'written' if changed else 'unchanged'}")
    except OSError as e:
        warn(f"could not write {file_version_txt}: {e}")

    # One always-on line: proves the script ran and shows the result.
    sys.stderr.write(
        f"VERSION INFO: Tag: {version_tag} | Build Time: {build_time}\n"
    )


# Runs at SCons load time so version.cpp exists before dependency scanning.
generate_version_header()
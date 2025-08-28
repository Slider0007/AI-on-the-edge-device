#!/usr/bin/env python3
"""
Post build tasks (only usable in context of platformio build pipeline)
- Preparing WebUI (tooltips, hashes, api docs)
- Create firmware package zip file (manual build only)
"""
import os
import sys
import subprocess
import time
import zipfile
import shutil

Import("env")

def postBuildAction(source, target, env):
    # -------------------------------------------------------------------------------------------------
    # Paths / Defines
    # -------------------------------------------------------------------------------------------------
    scriptName = "post_build_tasks.py"
    envName = env["PIOENV"]  # PlatformIO environment name (target)

    projectRoot = os.getenv("PROJECT_DIR", os.path.abspath(os.path.join(".", "..")))

    htmlSourceDir = os.path.join(projectRoot, "sd-card", "html")
    htmlTempDir = os.path.join(projectRoot, "sd-card", "html_compiled")

    # -------------------------------------------------------------------------------------------------
    # Detect if triggered by GitHub Actions
    # -------------------------------------------------------------------------------------------------
    inGithubActions = os.environ.get("GITHUB_ACTIONS", "").lower() == "true"
    if not inGithubActions:
        print(f"{scriptName}: Process post build tasks")
    else:
        print(f"{scriptName}: Process post build tasks (triggered by GitHub Action)")

    # -------------------------------------------------------------------------------------------------
    # Step 1: Prepare folders
    # -------------------------------------------------------------------------------------------------
    print(f"{scriptName}: Step 1: WebUI - Prepare folders")
    if os.path.exists(htmlTempDir):
        shutil.rmtree(htmlTempDir)

    shutil.copytree(htmlSourceDir, htmlTempDir)

    # -------------------------------------------------------------------------------------------------
    # Step 2: Generate parameter tooltips
    # -------------------------------------------------------------------------------------------------
    print(f"{scriptName}: Step 2: WebUI - Generate parameter tooltips")
    tooltipScript = os.path.join(projectRoot, "tools", "webui", "generate_param_tooltips.py")
    result = subprocess.run([sys.executable, tooltipScript, projectRoot], capture_output=True, text=True)
    if result.returncode != 0:
        print("Error generating tooltips: ")
        print(result.stdout)
        print(result.stderr)
        raise RuntimeError("Tooltip generation failed")

    # -------------------------------------------------------------------------------------------------
    # Step 3: Replace $COMMIT_HASH in all HTML files
    # -------------------------------------------------------------------------------------------------
    print(f"{scriptName}: Step 3: WebUI - Set hash in all HTML files")
    # Determine commitHash
    if inGithubActions:
        try:
            commitHash = (subprocess.check_output(["git", "rev-parse", "--short", "HEAD"], cwd=projectRoot).decode("utf-8").strip())
        except Exception:
            print(f"{scriptName}: Failed to parse git commit hash")
            commitHash = str(time.strftime("%y%m%d%H%M%S"))
    else:
        commitHash = str(time.strftime("%y%m%d%H%M%S"))

    # Replace $COMMIT_HASH
    for root, _, files in os.walk(htmlTempDir):
        for filename in files:
            if filename.endswith(".html"):
                filePath = os.path.join(root, filename)
                with open(filePath, "r", encoding="utf-8") as f:
                    content = f.read()
                content = content.replace("$COMMIT_HASH", commitHash)
                with open(filePath, "w", encoding="utf-8") as f:
                    f.write(content)

    # -------------------------------------------------------------------------------------------------
    # Step 4: Generate API docs
    # -------------------------------------------------------------------------------------------------
    print(f"{scriptName}: Step 4: WebUI - Generate API docs")
    scriptApiDocs = os.path.join(projectRoot, "tools", "webui", "generate_api_docs.py")
    result = subprocess.run([sys.executable, scriptApiDocs, projectRoot], capture_output=True, text=True)
    if result.returncode != 0:
        print("Error generating API docs: ")
        print(result.stdout)
        print(result.stderr)
        raise RuntimeError("API docs generation failed")

    # -------------------------------------------------------------------------------------------------
    # Step 5: Create ZIP after compilation (local only)
    # -------------------------------------------------------------------------------------------------
    if not inGithubActions:
        print(f"{scriptName}: Step 5: Create firmware package (ZIP file)")

        zipFilename = f"AI-on-the-edge-device__{envName}__SLFork_{commitHash}.zip"
        zipPath = os.path.join(projectRoot, zipFilename)

        def addDirectoryToZip(zipFile, directory, basePath, targetPrefix="", renameMap=None, excludeFiles=None):
            """
            Add a directory to a zip, keeping structure relative to basePath.
            Optionally remap top-level folder names with renameMap dict.
            """
            excludeFiles = excludeFiles or []
            renameMap = renameMap or {}

            for root, _, files in os.walk(directory):
                for fname in files:
                    if fname in excludeFiles:
                        continue
                    fullPath = os.path.join(root, fname)
                    relPath = os.path.relpath(fullPath, start=basePath)

                    parts = relPath.split(os.sep)
                    parts = [renameMap.get(p, p) for p in parts]
                    arcName = os.path.join(targetPrefix, *parts)

                    zipFile.write(fullPath, arcname=arcName)

        with zipfile.ZipFile(zipPath, "w", compression=zipfile.ZIP_DEFLATED) as zipFile:
            addDirectoryToZip(zipFile, os.path.join(projectRoot, "sd-card", "config"), projectRoot)
            addDirectoryToZip(zipFile, os.path.join(projectRoot, "sd-card", "html_compiled"), projectRoot, renameMap={"html_compiled": "html"})

            # Place binaries in zip root
            buildDir = env.subst("$BUILD_DIR")
            for binFile in ["bootloader.bin", "partitions.bin", "firmware.bin"]:
                fullPath = os.path.join(buildDir, binFile)
                zipFile.write(fullPath, arcname=binFile)

if "DISABLE_POST_BUILD_TASKS" not in os.environ: # Only run if it is not disabled in platformio.ini
    firmwareBinPath = os.path.join(env.subst("$BUILD_DIR"), "firmware.bin")
    env.AddPostAction(firmwareBinPath, postBuildAction)

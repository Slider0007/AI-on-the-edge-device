#!/usr/bin/env python3
"""
Grab all parameter files (markdown), convert them to HTML,
and compile them into a centralized JSON file for tooltips.
"""
import os
import glob
import shutil
import sys
import subprocess
import json
import gzip


# check for missing markdown package
def ensure_markdown():
    """Install the 'markdown' package on demand if it isn't available."""
    try:
        import markdown
    except ImportError:
        print("generate_param_tooltips.py: 'markdown' package not found, installing...")
        subprocess.check_call([sys.executable, "-m", "pip", "install", "markdown"])
        import markdown
    return markdown


markdown = ensure_markdown()


# Get root path (Parent folder of sd-card, tools, docs, ...)
try:
    Import("env")
    rootPath = os.getenv("PROJECT_DIR", os.path.abspath(os.path.join(".", "..")))
except Exception:
    # Standalone script execution
    if len(sys.argv) > 1:
        rootPath = os.path.abspath(sys.argv[1])
    else:
        print(f"Argument missing: Provide parent folder of sd-card folder")
        sys.exit(1)

htmlTempDir = os.path.join(rootPath, ".builds", "html_compiled")
parameterDocsFolder = os.path.join(rootPath, "docs", "Configuration", "Parameter")
htmlSourceDir = os.path.join(rootPath, "sd-card", "html")

# The new centralized JSON file destination
jsonOutPath = os.path.join(htmlTempDir, "tooltips.json")
jsonOutPathGz = os.path.join(htmlTempDir, "tooltips.json.gz")

# Pages to clean up
pagesToUpdate = ["edit_configuration.html", "edit_reference.html", "setup.html"]

# Dictionary to hold all documentation: { "section_parameter": "<html>...</html>" }
tooltip_database = {}

def update_html_triggers(pageFile, section, parameter):
    """
    Replaces old hidden placeholders with a modern lightweight trigger.
    The trigger uses data-help to reference the key in tooltips.json.
    """
    pagePath = os.path.join(htmlTempDir, pageFile)
    if not os.path.isfile(pagePath):
        return

    # This matches your existing HTML table structure
    search_pattern = f'<td style="visibility: hidden">$TOOLTIP_{section}_{parameter}</td>'
    replace_pattern = f'<td><div class="tooltip-trigger" data-help="{section}_{parameter}"></div></td>'

    # Handle the "hidden div" variant if present
    search_pattern_div = f'<div hidden>$TOOLTIP_{section}_{parameter}</div>'
    replace_pattern_div = f'<div class="tooltip-trigger" data-help="{section}_{parameter}" style="display:inline-block;"></div>'

    with open(pagePath, "r", encoding="utf-8") as f:
        content = f.read()

    content = content.replace(search_pattern, replace_pattern)
    content = content.replace(search_pattern_div, replace_pattern_div)

    with open(pagePath, "w", encoding="utf-8") as f:
        f.write(content)

def process_markdown_to_json(section, parameter, mdFile):
    """Converts MD to HTML and adds it to our JSON dictionary"""
    with open(mdFile, "r", encoding="utf-8") as f:
        mdContent = f.read()

    # Convert Markdown to HTML
    html = markdown.markdown(mdContent, extensions=["admonition", "tables"])

    # Clean up links for AI-on-the-edge docs
    html = html.replace("a href", "a target=_blank href")
    html = html.replace('href="../', 'href="https://jomjol.github.io/AI-on-the-edge-device-docs/')
    html = html.replace("../img/", "/")

    # Store in dict
    key = f"{section}_{parameter}"
    tooltip_database[key] = html

    # Update all relevant HTML pages to point to this key
    for page in pagesToUpdate:
        update_html_triggers(page, section, parameter)

# -------------------------------------------------------------------------------------------------
# Main Execution
# -------------------------------------------------------------------------------------------------
print(f"Generating modern centralized tooltips...")

# 1. Prepare HTML build directory and copy source HTML tree first
os.makedirs(htmlTempDir, exist_ok=True)
if os.path.isdir(htmlSourceDir):
    print(f"Prepare html folder...")
    shutil.copytree(htmlSourceDir, htmlTempDir, dirs_exist_ok=True)

# 2. Copy parameter-help images directly to htmlTempDir root to match root-relative URLs (e.g., ../img/ -> /)
paramImgFolder = os.path.join(parameterDocsFolder, "img")
if os.path.isdir(paramImgFolder):
    for imgFile in glob.glob(os.path.join(paramImgFolder, "*")):
        if os.path.isfile(imgFile):
            print(f"Prepare images folder...")
            shutil.copy2(imgFile, htmlTempDir)

# 3. Process parameter documentation markdown files
folders = sorted(d for d in glob.glob(os.path.join(parameterDocsFolder, "*")) if os.path.isdir(d))

for folder in folders:
    folderName = os.path.basename(folder)
    if folderName.lower() == "img":
        continue

    files = sorted(f for f in glob.glob(os.path.join(folder, "*.md")))
    for file in files:
        parameter = os.path.splitext(os.path.basename(file))[0].lower()
        parameter = parameter.replace("<", "").replace(">", "")
        process_markdown_to_json(folderName.lower(), parameter, file)

# Write JSON file
json_string = json.dumps(tooltip_database, indent=None) # Dropping indentation saves space before compression

# Raw tooltips (optional)
# with open(jsonOutPath, "w", encoding="utf-8") as f:
#     f.write(json_string)

# Gzipped tooltips
with gzip.open(jsonOutPathGz, "wt", encoding="utf-8", compresslevel=9) as f:
    f.write(json_string)

print(f"Success: {len(tooltip_database)} tooltips compiled into {jsonOutPathGz}")

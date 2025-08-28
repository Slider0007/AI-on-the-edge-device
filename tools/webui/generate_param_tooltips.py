#!/usr/bin/env python3
"""
Grab all parameter files (markdown) and convert them to HTML tooltips.
"""
import os
import glob
import markdown
import shutil
import sys

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

# Define HTML directory
htmlSourceDir = os.path.join(rootPath, "sd-card", "html")
htmlTempDir = os.path.join(rootPath, "sd-card", "html_compiled")

# Prepare folder (if not yet prepared by calling script)
if not os.path.exists(htmlTempDir):
    print(f"Prepare folders...")
    shutil.copytree(htmlSourceDir, htmlTempDir)

parameterDocsFolder = os.path.join(rootPath, "docs", "Configuration", "Parameter")

configPage = "edit_config_param.html"
referenceImagePage = "edit_reference.html"
setupSecurityPage = "setup_explain_5.html"

htmlTooltipPrefix = (
    '<div class="rst-content"><div class="tooltip">'
    '<img src="help.png" width="20px"><span class="tooltiptext">'
)
htmlTooltipSuffix = "</span></div></div>"


def injectTooltip(pageFile, search, replace):
    """Replace placeholders in a page with generated tooltip HTML"""
    pagePath = os.path.join(htmlTempDir, pageFile)
    if not os.path.isfile(pagePath):
        return

    with open(pagePath, "r", encoding="utf-8") as f:
        content = f.read()

    content = content.replace(search, replace)

    with open(pagePath, "w", encoding="utf-8") as f:
        f.write(content)


def generateHtmlTooltip(section, parameter, mdFile):
    """Convert markdown file to HTML tooltip and inject into pages"""
    with open(mdFile, "r", encoding="utf-8") as f:
        mdContent = f.read()

    htmlTooltip = markdown.markdown(mdContent, extensions=["admonition", "tables"])
    htmlTooltip = htmlTooltip.replace("a href", "a target=_blank href")
    htmlTooltip = htmlTooltip.replace(
        'href="../', 'href="https://jomjol.github.io/AI-on-the-edge-device-docs/'
    )
    htmlTooltip = htmlTooltip.replace("../img/", "/")
    htmlTooltip = htmlTooltipPrefix + htmlTooltip + htmlTooltipSuffix

    # Inject into relevant pages
    injectTooltip(
        configPage,
        f"<th hidden>$TOOLTIP_{section}_{parameter}</th>",
        f'<th style="font-weight: unset;">{htmlTooltip}</th>',
    )
    injectTooltip(
        configPage,
        f'<td style="visibility:hidden">$TOOLTIP_{section}_{parameter}</td>',
        f"<td>{htmlTooltip}</td>",
    )
    injectTooltip(
        referenceImagePage,
        f'<td style="visibility:hidden">$TOOLTIP_{section}_{parameter}</td>',
        f"<td>{htmlTooltip}</td>",
    )
    injectTooltip(
        setupSecurityPage,
        f'<td style="visibility:hidden">$TOOLTIP_{section}_{parameter}</td>',
        f"<td>{htmlTooltip}</td>",
    )


# -------------------------------------------------------------------------------------------------
# Generate parameter tooltips
# -------------------------------------------------------------------------------------------------
print(f"Generate parameter tooltips...")
folders = sorted(d for d in glob.glob(os.path.join(parameterDocsFolder, "*")) if os.path.isdir(d))

for folder in folders:
    folderName = os.path.basename(folder)

    files = sorted(f for f in glob.glob(os.path.join(folder, "*")) if os.path.isfile(f))

    if folderName.lower() == "img":
        for file in files:
            shutil.copy2(file, docsMainFolder)
    else:
        for file in files:
            if not file.endswith(".md"):
                continue
            parameter = os.path.splitext(os.path.basename(file))[0]
            parameter = parameter.replace("<", "").replace(">", "")
            generateHtmlTooltip(folderName.lower(), parameter.lower(), file)

print(f"Parameter tooltip generation completed")

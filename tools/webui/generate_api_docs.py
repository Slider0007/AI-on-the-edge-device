#!/usr/bin/env python3
"""
Grab all API doc files and create a single markdown file with flexible root path
"""
import os
import glob
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

docsApiRootFolder = os.path.join(rootPath, "docs", "API")

# Output markdown filenames
apiFiles = {
    "REST": "doc_api_rest.md",
    "MQTT": "doc_api_mqtt.md",
    "Prometheus-OpenMetrics": "doc_api_prometheus.md",
    "Webhook": "doc_api_webhook.md"
}

# Anchor prefixes for each API type (used for local links)
anchorPrefixes = {
    "REST": "rest-api",
    "MQTT": "mqtt-api",
    "Prometheus-OpenMetrics": "prometheus-api",
    "Webhook": "webhook-api"
}


def prepareApiMarkdown(markdownFile, apiType=None):
    """
    Read a markdown file, convert internal .md links to local anchors and fix image paths
    """
    with open(markdownFile, "r", encoding="utf-8") as f:
        content = f.read()

    if apiType in anchorPrefixes:
        prefix = anchorPrefixes[apiType]
        linkPosEnd = content.find(".md)")
        while linkPosEnd >= 0:
            replaceLink = content[content.rfind("(", 0, linkPosEnd) + 1:linkPosEnd + 3]
            replaceLinkName = replaceLink.split("\\")[-1].replace(".md", "")

            # Handle special cases
            if replaceLinkName == "_OVERVIEW":
                content = content.replace("_OVERVIEW.md", f"#overview-{prefix}")
            elif replaceLinkName == "xxx_migration_notes":
                content = content.replace("xxx_migration_notes.md", "#migration-notes")

            # Replace with local anchor
            if (apiType == "REST"):
                content = content.replace(replaceLink, f"#{prefix}-endpoint-{replaceLinkName}")
            else:
                content = content.replace(replaceLink, f"#{prefix}-{replaceLinkName}")

            linkPosEnd = content.find(".md)")

    # Update image paths
    content = content.replace("./img/", "/")

    return content

# -------------------------------------------------------------------------------------------------
# Generate API docs
# -------------------------------------------------------------------------------------------------
print(f"Generating API docs...")

folders = sorted(filter(os.path.isdir, glob.glob(os.path.join(docsApiRootFolder, "*"))))

# Collect all markdown content
markdownContents = {key: "" for key in apiFiles.keys()}

for folderPath in folders:
    folderName = os.path.basename(folderPath)
    files = sorted(f for f in glob.glob(os.path.join(folderPath, "*")) if os.path.isfile(f))

    for file in files:
        if not file.endswith(".md"):
            continue

        markdownContents.setdefault(folderName, "")
        markdownContents[folderName] += prepareApiMarkdown(file, apiType=folderName)

        # Add divider for REST and MQTT
        if folderName in ["REST", "MQTT"]:
            markdownContents[folderName] += "\n\n---\n"

    # Copy images to HTML folder
    imgFolder = os.path.join(folderPath, "img")
    if os.path.exists(imgFolder):
        for imgFile in sorted(glob.glob(os.path.join(imgFolder, "*"))):
            if os.path.isfile(imgFile):
                shutil.copy2(imgFile, htmlTempDir + "/")

# Write markdown files to HTML folder
for apiType, outputFile in apiFiles.items():
    outputPath = os.path.join(htmlTempDir, outputFile)
    with open(outputPath, "w", encoding="utf-8") as f:
        f.write(markdownContents.get(apiType, ""))

print(f"API docs generation completed")
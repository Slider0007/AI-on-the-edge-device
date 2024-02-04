"""
Grab all parameter files (markdown) and convert them to html files
"""
import os
import glob
import markdown
import shutil
import sys

scriptName = "generate-param-doc-tooltips-localbuild.py"

if len(sys.argv) > 1:
    scriptPath = sys.argv[1] + "/tools/parameter-tooltip-generator/"
    rootPath = sys.argv[1]
    
else:
    scriptPath = sys.argv[0].split(scriptName,1)[0]
    if scriptPath[0] == ".":
         scriptPath = os.getcwd() + scriptPath[1:]
    else:
        scriptPath = os.getcwd() + scriptPath

    rootPath = scriptPath.split("tools",1)[0]


parameterDocsFolder = rootPath + "/docs/Configuration/Parameter"
docsMainFolder = rootPath + "/sd-card/html"
configPage = "edit_config_param.html"

htmlTooltipPrefix = """
    <div class="rst-content"><div class="tooltip"><img src="help.png" width="28px"><span class="tooltiptext">
"""


htmlTooltipSuffix = """
    </span></div></div>
"""


folders = sorted( filter( os.path.isdir, glob.glob(parameterDocsFolder + '/*') ) )


def generateHtmlTooltip(section, parameter, markdownFile):
    # print(section, parameter, markdownFile)

    with open(markdownFile, 'r') as markdownFileHandle:
        markdownFileContent = markdownFileHandle.read()

    # Convert markdown files to HTML file
    htmlTooltip = markdown.markdown(markdownFileContent, extensions=['admonition', 'tables'])

    # Make all links to be opened in a new page
    htmlTooltip = htmlTooltip.replace("a href", "a target=_blank href")

    # Replace relative documentation links with absolute ones pointing to the external documentation
    htmlTooltip = htmlTooltip.replace("href=\"../", "href=\"https://jomjol.github.io/AI-on-the-edge-device-docs/")

    # Update image paths and copy images to right folder
    if "../img/" in htmlTooltip:
        htmlTooltip = htmlTooltip.replace("../img/", "/")

    htmlTooltip = htmlTooltipPrefix + htmlTooltip + htmlTooltipSuffix

    # Add the tooltip to the config page
    with open(docsMainFolder + "/" + configPage, 'r') as configPageHandle:
        configPageContent = configPageHandle.read()

    # print("replacing $TOOLTIP_" + section + "_" + parameter + " with the tooltip content...")
    configPageContent = configPageContent.replace("<td>$TOOLTIP_" + section + "_" + parameter + "</td>", "<td>" + htmlTooltip + "</td>")

    with open(docsMainFolder + "/" + configPage, 'w') as configPageHandle:
        configPageHandle.write(configPageContent)


print(scriptName + ": Generate tooltips")

"""
Generate a HTML tooltip for each markdown page
"""
for folder in folders:
    folder = folder.split("\\")[-1]  

    files = sorted(filter(os.path.isfile, glob.glob(parameterDocsFolder + "/" + folder + '/*')))

    if folder == "img":
        for file in files:
            shutil.copy2(file, docsMainFolder + "/")
    
    else:
        for file in files:
            if not ".md" in file: # Skip non-markdown files
                continue

            parameter = file.split("\\")[-1].replace(".md", "")
            parameter = parameter.replace("<", "").replace(">", "")
            generateHtmlTooltip(folder, parameter, file)

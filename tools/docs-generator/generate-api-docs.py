"""
Grab all API doc files and create a single markdown file
"""
import os
import glob
import shutil


docsAPIRootFolder = "./docs/API"
htmlFolder = "./sd-card/html"
docAPIRest = "doc_api_rest.md"
docAPIMqtt = "doc_api_mqtt.md"


# Generate REST API doc markdown file for offline usage
def prepareRestApiMarkdown(markdownFile):
    with open(markdownFile, 'r') as markdownFileHandle:
        markdownFileContent = markdownFileHandle.read()

    linkPosEnd = markdownFileContent.find(".md)")
    while (linkPosEnd >= 0):

        # Find markdown local links
        replaceLink = markdownFileContent[markdownFileContent.rfind("(", 0, linkPosEnd)+1:linkPosEnd+3]
        ReplaceLinkName = replaceLink.split("\\")[-1].replace(".md", "")

        # Taking care of special cases
        if (ReplaceLinkName == "_OVERVIEW"):
            markdownFileContent = markdownFileContent.replace("_OVERVIEW.md", "#overview-rest-api")
        elif (ReplaceLinkName == "xxx_migration_notes"):
            markdownFileContent = markdownFileContent.replace("xxx_migration_notes.md", "#migration-notes")
        
        # Replace all links with local links
        markdownFileContent = markdownFileContent.replace(replaceLink, "#rest-api-endpoint-" + ReplaceLinkName)

        # Find markdown local links
        linkPosEnd = markdownFileContent.find(".md)")

    # Update image paths
    if "./img/" in markdownFileContent:
        markdownFileContent = markdownFileContent.replace("./img/", "/")
    
    return markdownFileContent


# Generate MQTT API doc markdown file for offline usage
def prepareMqttApiMarkdown(markdownFile):
    with open(markdownFile, 'r') as markdownFileHandle:
        markdownFileContent = markdownFileHandle.read()

    linkPosEnd = markdownFileContent.find(".md)")
    while (linkPosEnd >= 0):
        
        # Find markdown local links
        replaceLink = markdownFileContent[markdownFileContent.rfind("(", 0, linkPosEnd)+1:linkPosEnd+3]
        ReplaceLinkName = replaceLink.split("\\")[-1].replace(".md", "")

        # Taking care of special cases
        if (ReplaceLinkName == "_OVERVIEW"):
            markdownFileContent = markdownFileContent.replace("_OVERVIEW.md", "#overview-mqtt-api")
        elif (ReplaceLinkName == "xxx_migration_notes"):
            markdownFileContent = markdownFileContent.replace("xxx_migration_notes.md", "#migration-notes")
        
        # Replace all links with local links
        markdownFileContent = markdownFileContent.replace(replaceLink, "#mqtt-api-" + ReplaceLinkName)

        linkPosEnd = markdownFileContent.find(".md)")

    # Update image paths
    if "./img/" in markdownFileContent:
        markdownFileContent = markdownFileContent.replace("./img/", "/")

    return markdownFileContent


##########################################################################################
# Generate API docs for offline usage in WebUI
##########################################################################################
print("Generating API docs...")

folders = sorted( filter( os.path.isdir, glob.glob(docsAPIRootFolder + '/*') ) )

markdownRestApi = ''
markdownMqttApi = ''

# Create a combined markdown file
for folder in folders:
    folder = folder.split("/")[-1]

    files = sorted(filter(os.path.isfile, glob.glob(docsAPIRootFolder + "/" + folder + '/*')))
    for file in files:
        if not ".md" in file: # Skip non-markdown files
            continue

        if (folder == "REST"):
            markdownRestApi += prepareRestApiMarkdown(file) # Merge files
            markdownRestApi += "\n\n---\n" # Add a divider line
        elif (folder == "MQTT"):
            markdownMqttApi += prepareMqttApiMarkdown(file) # Merge files
            markdownMqttApi += "\n\n---\n" # Add a divider line
    
    # Copy in API doc linked images to HTMl folder
    if os.path.exists(docsAPIRootFolder + "/" + folder + "/img"):
        files = sorted(filter(os.path.isfile, glob.glob(docsAPIRootFolder + "/" + folder + '/img/*')))
        for file in files:
            shutil.copy2(file, htmlFolder + "/")

# Write REST API markdown file
with open(htmlFolder + "/" + docAPIRest, 'w') as docAPIRestHandle:
    docAPIRestHandle.write(markdownRestApi)

# Write MQTT API markdown file
with open(htmlFolder + "/" + docAPIMqtt, 'w') as docAPIMqttHandle:
    docAPIMqttHandle.write(markdownMqttApi)
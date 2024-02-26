"""
Grab all parameter files (markdown) and convert them to html files
"""
import os
import glob
import shutil


docsAPIRootFolder = "./docs/API"
htmlFolder = "./sd-card/html"
docAPIRest = "doc_api_rest.md"
docAPIMqtt = "doc_api_mqtt.md"


# Generate REST API doc markdown file for offline usage
def generateRestAPI(markdownFile):
    with open(markdownFile, 'r') as markdownFileHandle:
        markdownFileContent = markdownFileHandle.read()

    linkPosEnd = markdownFileContent.find(".md)")
    while (linkPosEnd >= 0):
        print("REST while loop")

        # Find markdown local links
        replaceLink = markdownFileContent[markdownFileContent.rfind("(", 0, linkPosEnd)+1:linkPosEnd+3]
        ReplaceLinkName = replaceLink.split("\\")[-1].replace(".md", "")
        print(ReplaceLinkName)

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

    # Remove existing file
    if os.path.exists(htmlFolder + "/" + docAPIRest):
        os.remove(htmlFolder + "/" + docAPIRest)

    with open(htmlFolder + "/" + docAPIRest, 'a') as docAPIRestHandle:
        docAPIRestHandle.write(markdownFileContent)


# Generate MQTT API doc markdown file for offline usage
def generateMqttAPI(markdownFile):
    with open(markdownFile, 'r') as markdownFileHandle:
        markdownFileContent = markdownFileHandle.read()

    linkPosEnd = markdownFileContent.find(".md)")
    while (linkPosEnd >= 0):
        print("MQTT while loop")
        
        # Find markdown local links
        replaceLink = markdownFileContent[markdownFileContent.rfind("(", 0, linkPosEnd)+1:linkPosEnd+3]
        ReplaceLinkName = replaceLink.split("\\")[-1].replace(".md", "")
        print(ReplaceLinkName)

        # Taking care of special cases
        if (ReplaceLinkName == "_OVERVIEW"):
            markdownFileContent = markdownFileContent.replace("_OVERVIEW.md", "#overview-mqtt-api")
        
        # Replace all links with local links
        markdownFileContent = markdownFileContent.replace(replaceLink, "#mqtt-api-" + ReplaceLinkName)

        linkPosEnd = markdownFileContent.find(".md)")

    # Update image paths
    if "./img/" in markdownFileContent:
        markdownFileContent = markdownFileContent.replace("./img/", "/")

    # Remove existing file
    if os.path.exists(htmlFolder + "/" + docAPIMqtt):
        os.remove(htmlFolder + "/" + docAPIMqtt)

    with open(htmlFolder + "/" + docAPIMqtt, 'a') as docAPIMqttHandle:
        docAPIMqttHandle.write(markdownFileContent)


##########################################################################################
# Generate API docs for offline usage in WebUI
##########################################################################################
print("Generating API docs...")

folders = sorted( filter( os.path.isdir, glob.glob(docsAPIRootFolder + '/*') ) )

for folder in folders:
    folder = folder.split("/")[-1]
    print(folder)

    files = sorted(filter(os.path.isfile, glob.glob(docsAPIRootFolder + "/" + folder + '/*')))
    for file in files:
        print(file)
        if not ".md" in file: # Skip non-markdown files
            continue

        if (folder == "REST"):
            generateRestAPI(file)
        elif (folder == "MQTT"):
            generateMqttAPI(file)

    if os.path.exists(docsAPIRootFolder + "/" + folder + "/img"):
        files = sorted(filter(os.path.isfile, glob.glob(docsAPIRootFolder + "/" + folder + '/img/*')))
        for file in files:
            shutil.copy2(file, htmlFolder + "/")
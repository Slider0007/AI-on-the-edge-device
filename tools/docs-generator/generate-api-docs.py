"""
Grab all parameter files (markdown) and convert them to html files
"""
import os
import glob
import shutil


docsRootFolder = "./docs"
docsMainFolder = "./sd-card/html"
docAPIRest = "doc_api_rest.md"
docAPIMqtt = "doc_api_mqtt.md"


def generateRestAPI(markdownFile):
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

    with open(docsMainFolder + "/" + docAPIRest, 'a') as docAPIRestHandle:
        docAPIRestHandle.write(markdownFileContent)


def generateMqttAPI(markdownFile):
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
            
            # Replace all links with local links
            markdownFileContent = markdownFileContent.replace(replaceLink, "#mqtt-api-" + ReplaceLinkName)

            linkPosEnd = markdownFileContent.find(".md)")

    # Update image paths
    if "./img/" in markdownFileContent:
        markdownFileContent = markdownFileContent.replace("./img/", "/")

    with open(docsMainFolder + "/" + docAPIMqtt, 'a') as docAPIMqttHandle:
        docAPIMqttHandle.write(markdownFileContent)




print("Generating API docs...")

if os.path.exists(docsMainFolder + "/" + docAPIRest):
    os.remove(docsMainFolder + "/" + docAPIRest)

if os.path.exists(docsMainFolder + "/" + docAPIMqtt):
    os.remove(docsMainFolder + "/" + docAPIMqtt)

folders = sorted( filter( os.path.isdir, glob.glob(docsRootFolder + '/API/*') ) )

for folder in folders:
    folder = folder.split("/")[-1]

    files = sorted(filter(os.path.isfile, glob.glob(docsRootFolder + "/" + folder + '/*')))
    for file in files:
        if not ".md" in file: # Skip non-markdown files
            continue

        if (folder == "API\REST"):
            generateRestAPI(file)
        elif (folder == "API\MQTT"):
            generateMqttAPI(file)

    if os.path.exists(docsRootFolder + "/" + folder + "/img"):
        #os.makedirs(docsMainFolder + "/img/docs") 
        files = sorted(filter(os.path.isfile, glob.glob(docsRootFolder + "/" + folder + '/img/*')))
        for file in files:
            shutil.copy2(file, docsMainFolder + "/")
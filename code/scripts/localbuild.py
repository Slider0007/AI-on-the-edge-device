Import("env")

import os
import subprocess
import glob
import time

#############################
# Save path
actualPath = os.getcwd()


################################################################################################################################
print("localbuild.py: Generate HTML parameter tooltips")
#print(env["PROJECT_DIR"])
os.chdir('../')
subprocess.call("python ./tools/parameter-tooltip-generator/generate-param-doc-tooltips-localbuild.py " + os.getcwd())


################################################################################################################################
print("localbuild.py: Add hash to HTML files")
os.chdir('sd-card/html')

files = sorted(filter(os.path.isfile, glob.glob('*')))
hash = str(round(time.time()*1000)) # timestamp in miliseconds

for file in files:
    if not ".html" in file: # Skip non-HTML files
        continue
    
    filename = os.getcwd() + "/" + file
    with open(filename) as file:
        content = file.read()
    
    content = content.replace("$COMMIT_HASH", hash)

    with open(filename, 'w') as file:
        file.write(content)


#############################
# Restore path
os.chdir(actualPath)

print("localbuild.py: Tasks completed")
#!/bin/bash

# Checkout the documentation repo branch which hosts the parameter description (tooltip on configuration page)
if [ -d "AI-on-the-edge-device-docs" ] ; then
    # Repo already checked out, pull it
    cd AI-on-the-edge-device-docs
    git checkout parameter-description
    git pull
    cd ..
else
    # Repos folde ris missing, clone it
    git clone https://github.com/Slider0007/AI-on-the-edge-device-docs.git
fi

python generate-param-doc-tooltips.py

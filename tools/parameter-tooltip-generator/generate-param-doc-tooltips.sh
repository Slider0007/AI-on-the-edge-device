#!/bin/bash

# Checkout the documentation repo branch which hosts the parameter description (tooltip on configuration page)
git clone https://github.com/Slider0007/AI-on-the-edge-device-docs.git
git checkout parameter-description
git pull

python generate-param-doc-tooltips.py

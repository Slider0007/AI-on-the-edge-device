# Parameter: ROI Images Saving Size

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | ROI Images Saving Size | roisavingsize
| Default Value     | `Full`              | `0`
| Input Options     | `Full`<br>`Resized`<br>`Full + Resized` | `0`<br>`1`<br>`2`


## Description

Defines in which size the digit ROI images are saved to SD card


### Input Options

| Input Option     | Description
|:---              |:---
| `Full`           | Saves the image in its original size (as defined by the configured ROI)<br>Path: `/log/digit/{DATE}/{HOUR}/`<br>Filename: `x.y_sequenceName_roiName_dateTime.jpg`
| `Resized`        | Saves the image resized to the model's input dimensions<br>Path: `/log/digit/{DATE}/{HOUR}/`<br>Filename: `x.y_sequenceName_roiName_resized_dateTime.jpg`
| `Full + Resized` | Saves both the original and resized versions<br>Path: `/log/digit/{DATE}/{HOUR}/` + subfolder `/resized`<br>Filenames: `x.y_sequenceName_roiName_dateTime.jpg` and `/resized/x.y_sequenceName_roiName_resized_dateTime.jpg`


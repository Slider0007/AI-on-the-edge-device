# Parameter: ROI Saving Size

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | ROI Saving Size     | roisavingsize
| Default Value     | `Full`              | `0`
| Input Options     | `Full`<br>`Resized`<br>`Full + Resized` | `0`<br>`1`<br>`2`


## Description

Defines in which size the digit ROI images are saved to SD card


### Input Options

| Input Option     | Description
|:---              |:---
| `Full`           | Save original image size (Size depending on configured ROI size)<br>Path: e.g. `/log/digit/{DATE}/{HOUR}/`<br>Name: `x.y_sequenceName_roiName_dateTime.jpg`
| `Resized`        | Save ROI image resized to model input size<br>Path: `/log/digit/{DATE}/{HOUR}/`<br>Name: `x.y_sequenceName_roiName_resized_dateTime.jpg`
| `Full + Resized` | Save both sizes<br>Path: e.g. `/log/digit/{DATE}/{HOUR}/` + subfolder `/resized`<br>Names: `x.y_sequenceName_roiName_dateTime.jpg` + `x.y_sequenceName_roiName_resized_dateTime.jpg`


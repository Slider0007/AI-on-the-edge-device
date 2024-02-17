# Parameter: Zoom Mode

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Zoom Mode           | ZoomMode
| Default Value     | `Crop`              | `0`
| Input Options     | `Crop`<br>`Scale & Crop` | `0`<br>`1`


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!


## Description

Select the zoom mode. 
This only applies when `Digital Zoom` is enabled.


| Input Option  | Description
|:---           |:---
| `Crop`        | Crop the high resolution camera sensor frame to `ImageSize` resolution
| `Scale & Crop`| Scale the high resolution camera sensor frame first to 800 x 600 pixels then crop it to `ImageSize` resolution


!!! Tip
    This parameter should be set on the 'Reference Image' configuration page. 
    There you have a visual feedback.

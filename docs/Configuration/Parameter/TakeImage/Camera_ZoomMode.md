# Parameter: Zoom Mode

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Zoom Mode           | zoommode
| Default Value     | `Off`               | `0`
| Input Options     | `Off`<br>`Crop`<br>`Scale & Crop` | `0` .. `2`


## Description

Select the digital zoom (image windowing) mode.


| Input Option  | Description
|:---           |:---
| `Off`         | Digital zoom (image windowing) off. Image with `ImageSize` will be directly processed.
| `Crop`        | Crop an area (`ImageSize`, default: 640 x 480) out of the high resolution image (1600 x 1200). 
| `Scale & Crop`| Scale down the high resolution image (1600 x 1200) to 800 x 600 and crop an area (`ImageSize`, default: 640 x 480) out of 800 x 600 image.


!!! Note
    Position of area can be defined with `Zoom Offset X` and `Zoom Offset Y`.


!!! Tip
    This parameter should be set on the 'Reference Image' configuration page. 
    There you have a visual feedback.

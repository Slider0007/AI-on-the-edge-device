# Parameter: Zoom Offset X

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Zoom Offset X       | zoomoffsetx
| Default Value     | `0`                 | `0`
| Input Options     | min. `-960` .. max. `960`| `-960` .. `960`
| Unit              | Pixel               | Pixel


## Description

X coordinate of the crop location within the camera sensor frame. `0` equals to 
the top-left coordinate of the centered image.


!!! Note
    The parameter input limits are updated depending on actual zoom factor.<br>
    The more of the whole camera sensor is visible (equals to low zoom factor) 
    the lower are the possiblities (and limits) to adjust.


!!! Tip
    This parameter should be set on the 'Reference Image' configuration page. 
    There you have a visual feedback. This paramter only applies when `Zoom Factor`
    is higher than `1.0x`.

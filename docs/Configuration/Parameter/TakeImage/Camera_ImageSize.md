# Parameter: Image Size

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Image Size          | imagesize
| Default Value     | `VGA`               | `VGA`
| Input Options     | `VGA` (640 x 480 Pixel)<br>`QVGA` (320 x 240 Pixel) | `VGA`<br>`QVGA`


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!  


## Description

Size of the raw camera image taken by the camera module.


!!! Note
    The camera module itself would support larger sizes than 640 x 480 pixel but it's not 
    possible to process larger images due to RAM limitation. 

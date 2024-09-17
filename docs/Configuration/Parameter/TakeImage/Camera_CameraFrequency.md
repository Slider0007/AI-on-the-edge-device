# Parameter: Camera Frequency

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Camera Frequency    | camerafrequency
| Default Value     | `20`                | `20`
| Input Options     | `5 Mhz`<br>`8 Mhz`<br>`10 Mhz`<br>`20 Mhz` | `5`<br>`8`<br>`10`<br>`20`
| Unit              | Mhz                 | Mhz


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!  


## Description

This parameter sets the internally used camera frequency.


!!! Note
    Usually there is no need to adapt this setting. Some ESP32CAM modules do have issues
    with bad responsivness. One possible root cause could be possible disturbances due to
    bad PCB routing of the frequency wire from SOC to the attached camera module. Modifying the
    camera frequency could possibly improve the situation. If this is not improving only hardware
    related optimizations are possible like shielding the respective area by copper foil, etc...
    

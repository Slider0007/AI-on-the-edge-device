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

Set the camera frequency


!!! Note
    The camera frequency could have negative impact on wireless connection quality 
    (interference with WLAN signal), especially using low quality boards or boards 
    with onboard antenna. Depending on used hardware combination, try to find the 
    best camera frequency (default: 20Mhz) under the evaluation of network 
    responiveness and resulting image quality.<br>
    If this is not improving the situation only hardware related optimizations are 
    possible like shielding the respective area by copper foil, etc...
    

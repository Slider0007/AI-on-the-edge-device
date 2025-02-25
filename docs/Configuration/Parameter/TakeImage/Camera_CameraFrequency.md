# Parameter: Camera Frequency

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Camera Frequency    | camerafrequency
| Default Value     | `10`                | `10`
| Input Options     | `6 Mhz`<br>`8 Mhz`<br>`10 Mhz`<br>`15 Mhz`<br>`20 Mhz` | `6`<br>`8`<br>`10`<br>`15`<br>`20`
| Unit              | Mhz                 | Mhz


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!  


## Description

Set the camera frequency


!!! Note
    The camera clock frequency might have negative impact (interfere with WLAN signal) 
    on wireless network responsiveness (slow loading WebUI, higher latency), especially 
    using low quality boards or boards with onboard antenna. Depending on used hardware 
    combination and WIFI channel, try to find the best camera clock frequency under the 
    evaluation of network responsiveness and resulting image quality.<br>
    If this is not improving the situation only hardware related optimizations are 
    possible like shielding the respective area by copper foil, etc...
    

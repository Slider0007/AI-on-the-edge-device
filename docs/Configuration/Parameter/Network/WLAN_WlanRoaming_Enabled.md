# Parameter: Roaming

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Roaming             | enabled
| Default Value     | `Disabled`          | `false`
| Input Options     | `Disabled`<br>`Enabled` | `false`<br>`true` 


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!


## Description

This parameter activates a client triggered AP switching functionality (simplified roaming). 
If actual RSSI value is lower (more negative) than this parameter, all WIFI channels will be
scanned for configured access point SSID. If an access point is in range which has better 
RSSI value (less negative) than actual RSSI value + 5 dBm, the device is trying to connect 
to this access point with the better RSSI value.


!!! Note
    The RSSI check only gets initiated at the end of each processing cycle to avoid any 
    disturbance of image processing.

# Parameter: WIFI Roaming RSSI Threshold

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | WIFI Roaming RSSI Threshold | RSSIThreshold
| Default State     | `disabled` -> unselected | `disabled` -> ;
| Default Value     | `-75`               | `-75`
| Input Options     | `-100` .. `0`       | `-100` .. `0` 
| Unit              | dBm                 | dBm


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!


## Description

This parameter activates a client triggered AP switching functionality (simplified roaming). 
If actual RSSI value is lower (more negative) than this parameter, all WIFI channels will be scanned for configured access point SSID. If an access point is in range which has better RSSI value (less negative) than actual RSSI value + 5 dBm, the device is trying to connect to this access point with the better RSSI value.


!!! Note
    The RSSI check only gets initiated at the end of each processing cycle to avoid any disturbance of image processing.


!!! Note
     This parameter gets additionally saved to `/wlan.ini` file on the SD-Card during next startup.

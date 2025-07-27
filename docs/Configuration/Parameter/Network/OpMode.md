# Parameter: Operation Mode

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Operation Mode      | opmode
| Default Value     | `WLAN Client`       | `0`
| Input Options     | `Disabled`<br>`WLAN CLient`<br>`WLAN Client (Timed-Off)`<br>`WLAN Access Point`<br>`WLAN Access Point (Timed-Off)`<br>`Ethernet`<br>`Ethernet (Fallback WLAN AP)` | `-1`<br>`0`<br>`1`<br>`2`<br>`3`<br>`4`<br>`5`


## Description

Select the network operation mode


| Input Option               | Description
|:---                        |:---
| `Disabled`                 | All network connections are disabled (no interaction with device possible, but device is processing).
| `WLAN Client`              | WLAN connection is established to a wireless network in range.
| `WLAN Client (Timed-Off)`  | WLAN connection is established to a wireless network in range. Network is suspended after a configurable time is elapsed and actual cycle processing is completed (Parameter: Timed-Off Delay).
| `WLAN Access Point`        | Standalone mode. Device is providing an access point.
| `WLAN Access Point (Timed-Off)` | Standalone mode. Device is providing an access point. Access point is suspended after a configurable time is elapsed, no client is connected and actual cycle processing is completed (Parameter: Timed-Off Delay).
| `Ethernet` | Use ethernet connection. Only available for devices with ethernet interface
| `Ethernet (Fallback WLAN AP)` | Use ethernet connection. If no ethernet connection is established within 30s, device's WLAN Access Point will be activated. Only available for devices with ethernet interface

!!! Note
    To apply this parameter a device reboot is required.


!!! Tip
    A suspended network connection can be resumed by GPIO using option `Resume WLAN connection`. 
    This can be configured in `GPIO` section
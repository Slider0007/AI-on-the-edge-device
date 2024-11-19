# Parameter: Operation Mode

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Operation Mode      | opmode
| Default Value     | `WLAN Client`       | `0`
| Input Options     | `Disabled`<br>`WLAN CLient`<br>`WLAN Client (Timed-Off)`<br>`WLAN Access Point`<br>`WLAN Access Point (Timed-Off)` | `-1`<br>`0`<br>`1`<br>`2`<br>`3`


## Description

Select the network operation mode


| Input Option               | Description
|:---                        |:---
| `Disabled`                 | All network connections are disabled (no interaction with device possible, but device is processing).
| `WLAN Client`              | WLAN connection is established to a wireless network in range.
| `WLAN Client (Timed-Off)`  | WLAN connection is established to a wireless network in range. Network is suspended after a configurable time is elapsed and actual cycle processing is completed (Parameter: Timed-Off Delay).
| `WLAN Access Point`        | Standalone mode. Device is providing an access point.
| `WLAN Access Point (Timed-Off)` | Standalone mode. Device is providing an access point. Access point is suspended after a configurable time is elapsed, no client is connected and actual cycle processing is completed (Parameter: Timed-Off Delay).


!!! Note
    To apply this parameter a device reboot is required.


!!! Tip
    A suspended network connection can be resumed by GPIO using option `Resume WLAN connection`. 
    This can be configured in `GPIO` section
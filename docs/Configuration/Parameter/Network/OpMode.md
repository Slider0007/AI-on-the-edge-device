# Parameter: Operation Mode

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Operation Mode      | opmode
| Default Value     | `WLAN Client`<br>`Ethernet (Fallback WLAN AP)`* | `0`<br>`5`
| Input Options     | `Disabled`<br>`WLAN CLient`<br>`WLAN Client (Timed-Off)`<br>`WLAN Access Point`<br>`WLAN Access Point (Timed-Off)`<br>`Ethernet`<br>`Ethernet (Fallback WLAN AP)` | `-1`<br>`0`<br>`1`<br>`2`<br>`3`<br>`4`<br>`5`


## Description

Defines the device’s network interface


| Input Option               | Description
|:---                        |:---
| `Disabled`                 | All network interfaces are disabled. Device continues internal processing
| `WLAN Client`              | Connects to a configured Wi-Fi network.
| `WLAN Client (Timed-Off)`  | Connects to a configured Wi-Fi network. Connection is suspended after a configurable time is elapsed and actual cycle processing is completed (Parameter: Timed-Off Delay)
| `WLAN Access Point`        | Activates the device's own Wi-Fi AP. A client is able to connect to provided AP to interact with device
| `WLAN Access Point (Timed-Off)` | Activates the device's own Wi-Fi AP. A client is able to connect to provided AP to interact with device. Access point is suspended after a configurable time is elapsed, no client is connected and actual cycle processing is completed (Parameter: Timed-Off Delay)
| `Ethernet`* | Connects via wired ethernet
| `Ethernet (Fallback WLAN AP)`* | Connects via wired ethernet. If not able to connected within 30 seconds, device's own Wi-Fi AP will be activated as fallback.

*Only available on devices with an ethernet interface

!!! Note
    To apply this parameter a device reboot is required.


!!! Tip
    A suspended network connection can be resumed by GPIO using option `Resume WLAN connection`. 
    This can be configured in `GPIO` section
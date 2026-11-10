# Parameter: Power Mode

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Power Mode          | powermode
| Default Value     | `Power Saving DTIM (Default)` | `1`
| Input Options     | `Power Saving Disabled`<br>`Power Saving DTIM (Default)`<br>`Power Saving Interval` | `0`<br>`1`<br>`2`


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!  


## Description

Set the WIFI client power mode (station only; not applicable for access point mode)


### Input Options

| Input Option     | Description
|:---              |:---
| `Power Saving Disabled`| Sleep mode disabled. Lowest latency, but highest power consumption / heat generation -> Not recommended! Use with care!
| `Power Saving DTIM (Default)`| Sleep mode activated. Wakeup dynamically syncs with the AP's DTIM period (synced to broadcast/multicast schedules / most APs: ~100ms) -> Medium latency, reduced power consumption.
| `Power Saving Interval`| Sleep mode activated. Wakeup occurs on a fixed client-defined listen interval (hard-coded ESP-IDF default: 3 AP beacon intervals / ~300ms), completely independent of the router's DTIM schedule. Latency is higher and broadcast/multicast data can be missed, but it yields maximum power savings.


!!! Note
    To apply this parameter a device reboot is required
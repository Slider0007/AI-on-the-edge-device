# Parameter: Power Mode

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Power Mode          | powermode
| Default Value     | `Power Saving DTIM (Default)` | `1`
| Input Options     | `Power Saving Disabled`<br>`Power Saving DTIM (Default)`<br>`Power Saving Interval` | `0`<br>`1`<br>`2`


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!  


## Description

Set the WIFI client power mode (not applicable for access point operation (standalone))


### Input Options

| Input Option     | Description
|:---              |:---
| `Power Saving Disabled`| Sleep mode disabled. Lowest latency, but highest consumption / heat generation -> Not recommended! Use with care! 
| `Power Saving DTIM (Default)`| Sleep mode activated. Wakeup based on DTIM interval (usually every 100ms, depending on router/AP) -> Medium latency, reduced power consumption
| `Power Saving Interval`| Sleep mode activated. Wakeup based on client defined fixed listen interval (DTIMs or broadcast data could be missed) -> Higher latency, lowest power consumption


!!! Note
    To apply this parameter a device reboot is required
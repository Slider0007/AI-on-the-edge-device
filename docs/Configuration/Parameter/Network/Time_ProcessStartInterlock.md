# Parameter: Process Start Interlock

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Process Start Interlock | processstartinterlock
| Default Value     | `Enabled`           | `true`
| Input Options     | `Disabled`<br>`Enabled` | `false`<br>`true` 


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!  


## Description

The process will only start when a valid system time is available, ensuring accurate and 
reliable result documentation.


!!! Note
    The device loses its internal system time after a power loss. However, 
    the time is retained in memory after a regular reboot.

!!! Tip
    It's **recommended** to use NTP time synchronization and keep this enabled.
# Parameter: Process Start Interlock

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Process Start Interlock | Processstartinterlock
| Default Value     | `Enabled`           | `true`
| Input Options     | `Disabled`<br>`Enabled` | `false`<br>`true` 


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!  


## Description

Process starts only with valid time to ensure proper and plausible result documentation. 


!!! Note
    The device looses the intenal system time after power loss. 
    After reboot the time is kept in memory, though.

!!! Tip
    It's **recommended** to use time synchronization and this interlock.
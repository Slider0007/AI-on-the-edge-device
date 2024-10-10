# Parameter: Time Synchronization

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Time Synchronization | timesyncenabled
| Default Value     | `Enabled`           | `true`
| Input Options     | `Disabled`<br>`Enabled` | `false`<br>`true`


## Description

Enable or disable NTP time synchronization service.
If synchronization is disabled, the device's time starts always with 
time `01.01.1970 00:00:00` after boot.


!!! Note
    The device looses the intenal system time after power loss. 
    After reboot the time is kept in memory, though.

!!! Tip
    It's **recommended** to use time synchronization. 
    Most likely it's possible to set the address to the local IP of your router. 
    Many routers like FritzBox acts as a local NTP server.
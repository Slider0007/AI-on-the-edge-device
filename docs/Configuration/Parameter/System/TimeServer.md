# Parameter: Time Server (NTP)

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Time Server (NTP)   | TimeServer
| Default State     | `enabled` -> selected | `enabled` -> no ;
| Default Value     | `pool.ntp.org`      | `pool.ntp.org`


## Description

NTP time server which is used to synchronize the internal system time.<br>
If this option is disabled, no time synchronization will be performed. Then time always starts at `01.01.1970` after each power cycle!

!!! Note
    The device looses the intenal system time after power loss. After reboot the time is kept in memory, though.


!!! Tip
    Most likely it's possible to set the address to the local IP of your router. Many routers like Fritzboxes acts as a local NTP server.


# Parameter: Time Server

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Time Server         | timeserver
| Default Value     | empty --> `pool.ntp.org` | empty


## Description

NTP time server which is used to synchronize the device system time. 
If this parameter is empty, `pool.ntp.org` will be used.


!!! Note
    The device looses the intenal system time after power loss. 
    After reboot the time is kept in memory, though.


!!! Tip
    It's **recommended** to use time synchronization. 
    Most likely it's possible to set the address to the local IP of your router. 
    Many routers like FritzBox acts as a local NTP server.


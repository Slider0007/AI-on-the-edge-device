# Parameter: Password

|                   | WebUI               | Firmware
|:---               |:---                 |:----
| Parameter Name    | Password            | password
| Default Value     | empty               | empty


## Description

Password for the InfluxDB authentication.


!!! Note
    The password gets saved to NVS storage for security reason. After initial saving 
    the password is not accessible anymore, neither by WebUI nor by any API. As indication 
    for a password set, dots are displayed as placeholder. An empty password results in an 
    empty  parameter field, though.

!!! Warning
    The initial transmission is not encrypted and password is sent as cleartext. 

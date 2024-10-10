# Parameter: Password

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Password            | password
| Default Value     | empty               | empty


## Description

Password for MQTT broker authentication.


!!! Note
    The password gets saved to NVS storage for security reason. After initial saving 
    the password is not accessible anymore, neither by WebUI nor by any API. As indication 
    for a password set, dots are displayed as placeholder. An empty password results in an 
    empty  parameter field, though.

!!! Warning
    During initial transmission password is sent as cleartext. 

# Parameter: Token

|                   | WebUI               | Firmware
|:---               |:---                 |:----
| Parameter Name    | Token               | token
| Default Value     | empty               | empty


## Description

Token (similar to a password) for the InfluxDB authentication.

!!! Note
    The token gets saved to NVS storage for security reason. After initial saving 
    the token is not accessible anymore, neither by WebUI nor by any API. As indication 
    for a password set, dots are displayed as placeholder. An empty token results in an 
    empty parameter field, though.

!!! Warning
    The initial transmission is not encrypted and password is sent as cleartext. 

# Parameter: URI

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | URI                 | Uri
| Default Value     | empty               | empty


## Description

URI (protocol, IP-address and port) of the MQTT broker (server).<br>
- Connection unencrypted: e.g. `mqtt://192.168.1.x:1883`<br>
- Connection using TLS encryption: e.g. `mqtts://192.168.1.x:8883`<br>

!!! Note
    Only MQTT version 3.1.x is supported up to now.

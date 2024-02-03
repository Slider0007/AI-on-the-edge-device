# Parameter: URI

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | URI                 | Uri
| Default Value     | `http://IP-ADDRESS:1883` | `http://IP-ADDRESS:1883`


## Description

URI (protocol, IP-address and port) of the MQTT broker (server).<br>
Connection unencrypted: e.g. `mqtt://192.168.1.1:1883`<br>
Connection using TLS encryption: e.g. `mqtts://192.168.1.1:8883`

!!! Note
    Only MQTT version 3.1.1 is supported up to now.

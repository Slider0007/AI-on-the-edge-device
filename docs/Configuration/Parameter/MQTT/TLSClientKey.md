# Parameter: Client Key

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Client Key          | TLSClientKey
| Default Value     | `/config/certs/client.key` | `/config/certs/client.key`


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!


## Description

Location of client private key file (absolute path in relation to sdcard root)<br>


The client private key is used for TLS handshake of MQTT broker authentification. The client certificate and 
related client private key is used by the MQTT client to prove its identity to the MQTT broker (server).

!!! Note
    Typical file extention: `*.key`, `*.pem`<br>
    Only unencrypted and not password protected files are supported.


!!! Note
    Using TLS for MQTT, adaptions of MQTT `URI` parameter needs to be done, as well.  Please ensure suitable MQTT
    TLS protocol `mqtts://` and proper MQTT TLS port selection. e.g. `mqtts://IP-ADDRESS:8883`

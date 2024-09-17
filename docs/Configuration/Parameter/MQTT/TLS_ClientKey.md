# Parameter: Client Key

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Client Key          | TLSClientKey
| Default Value     | empty               | empty


## Description

Location of client private key file (absolute path in relation to sdcard root)<br>


The client private key is used for TLS handshake of MQTT broker authentification. The client certificate and 
related client private key is used by the MQTT client to prove its identity to the MQTT broker (server).

!!! Note
The certificate file needs to be copied to SD card folder `/config/certs`.<br>
    Typical file extention: `*.key`, `*.pem`<br>
    Only unencrypted and not password protected files are supported.


!!! Note
    Using TLS for MQTT, adaptions of MQTT `URI` parameter needs to be done, as well.  Please ensure suitable MQTT
    TLS protocol `mqtts://` and proper MQTT TLS port selection. e.g. `mqtts://IP-ADDRESS:8883`

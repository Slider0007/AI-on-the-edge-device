# Parameter: Client Certificate

|                   | WebUI               | Firmware
|:---               |:---                 |:----
| Parameter Name    | Client Certificate  | TLSClientCert
| Default Value     | empty               | empty


## Description

Location of client certificate file (absolute path in relation to sdcard root)


The client certificate is used for TLS handshake of MQTT broker authentification. The client certificate and 
related client private key is used by the MQTT client to prove its identity to the MQTT broker (server).

!!! Note
The certificate file needs to be copied to SD card folder `/config/certs`.<br>
    Typical file extentions: `*.crt`, `*.pem`, `*.der`<br>
    Only unencrypted and not password protected files are supported.


!!! Note
    Using TLS for MQTT, adaptions of MQTT `URI` parameter needs to be done, as well.  Please ensure suitable MQTT
    TLS protocol `mqtts://` and proper MQTT TLS port selection. e.g. `mqtts://IP-ADDRESS:8883`

# Parameter: Client Certificate

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Client Certificate  | TLSClientCert
| Default Value     | `/config/certs/client.crt` | `/config/certs/client.crt`


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!


## Description

Location of client certificate file (absolute path in relation to sdcard root)


The client certificate is used for TLS handshake of MQTT broker authentification. The client certificate and 
related client private key is used by the MQTT client to prove its identity to the MQTT broker (server).

!!! Note
    Typical file extentions: `*.crt`, `*.pem`, `*.der`<br>
    Only unencrypted and not password protected files are supported.


!!! Note
    Using TLS for MQTT, adaptions of MQTT `URI` parameter needs to be done, as well.  Please ensure suitable MQTT
    TLS protocol `mqtts://` and proper MQTT TLS port selection. e.g. `mqtts://IP-ADDRESS:8883`

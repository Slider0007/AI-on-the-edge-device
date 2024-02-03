# Parameter: CA Certificate

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | CA Certificate      | TLSCACert
| Default Value     | `/config/certs/ca.crt` | `/config/certs/ca.crt`


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!


## Description

Location of CA (Certificate Authority) certificate file (absolute path in relation to sdcard root)


The CA certificate is used for TLS handshake of MQTT broker authentification. The CA certificate is 
used by the client to validate the broker is who it claims to be.


!!! Note
    Typical file extentions: `*.crt`, `*.pem`, `*.der`<br>
    Only unencrypted and not password protected files are supported.<br>

    
!!! Note
    Certificate CN field (common name) check is disabled by default (hard-coded).


!!! Note
    Using TLS for MQTT, adaptions of MQTT `URI` parameter needs to be done, as well.  Please ensure suitable MQTT
    TLS protocol `mqtts://` and proper MQTT TLS port selection. e.g. `mqtts://IP-ADDRESS:8883`

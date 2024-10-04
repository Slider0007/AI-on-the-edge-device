# Parameter: CA Certificate

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | CA Certificate      | cacert
| Default Value     | empty               | empty


## Description

CA (Certificate Authority) certificate file (file name with extention, file placed in /config/certs).


The CA certificate is used for TLS handshake of MQTT broker authentification. The CA certificate is 
used by the client to validate the broker is who it claims to be.


!!! Note
The certificate file needs to be copied to SD card folder `/config/certs`.<br>
    Supported formats:<br>
    - `PEM` (Base64-ASCII-coding, File extentions: `.pem, .crt, .cer`)<br>
    - `DER` (Binary coding, File extention: `.der, .cer`)<br>
    Only unencrypted and not password protected files are supported.

    
!!! Warning
    Certificate CN field (common name) check is disabled by default (hard-coded).


!!! Note
    Using TLS for MQTT, adaptions of MQTT `URI` parameter needs to be done, as well.  Please ensure suitable MQTT
    TLS protocol `mqtts://` and proper MQTT TLS port selection. e.g. `mqtts://IP-ADDRESS:8883`

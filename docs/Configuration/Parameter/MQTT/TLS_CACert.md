# Parameter: CA Certificate

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | CA Certificate      | cacert
| Default Value     | empty               | empty


## Description

Select CA (Certificate Authority) certificate file.<br>
The CA certificate is used for TLS handshake of MQTT broker authentification. The CA certificate is 
used by the client to validate the broker is who it claims to be.


!!! Note
    The certificate file needs to be copied to SD card folder `/config/certs`.<br>
    Supported formats:<br>
    - `PEM` (Base64-ASCII-coding, File extentions: `.pem, .crt, .cer`)<br>
    - `DER` (Binary coding, File extention: `.der, .cer`)<br>
    Only unencrypted and not password protected files are supported.


!!! Tip
    If no custom certificate file is selected, built-in certificate bundle is used by default. 
    The bundle comes with a full list of root certificates from Mozilla's NSS root certificate store. 

    
!!! Warning
    Certificate CN field (common name) check is disabled by default (hard-coded).


# Parameter: Client Certificate

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Client Certificate  | clientcert
| Default Value     | empty               | empty


## Description

Client certificate file for mutual authentication (file name with extention, file placed in /config/certs). 
Keep it empty if mutual authentication is not required. If configured, `Client Key` needs to be provided, too.

The client certificate is used for TLS handshake of InfluxDB mutual authentification. The client certificate and 
related client private key is used by the HTTP client to prove its identity to the InfluxDB server.

!!! Note
The certificate file needs to be copied to SD card folder `/config/certs`.<br>
    Supported formats:<br>
    - `PEM` (Base64-ASCII-coding, File extentions: `.pem, .crt, .cer`)<br>
    - `DER` (Binary coding, File extention: `.der, .cer`)<br>
    Only unencrypted and not password protected files are supported.


!!! Note
    Using TLS for InfluxDB, adaptions of InfluxDB `URI` parameter needs to be done, as well.  Please ensure 
    protocol `https://` is configured, e.g. `https://IP-ADDRESS:8086`

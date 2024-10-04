# Parameter: Client Key

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Client Key          | clientkey
| Default Value     | empty               | empty


## Description

Client private key file (file name with extention, file placed in /config/certs)<br>

Client private key file for mutual authentication (absolute path in relation to sd card root folder). 
If configured, `Client Certificate` needs to be configured, too.

The client private key is used for TLS handshake of InfluxDB authentification. The client certificate and 
related client private key is used by the HTTP client to prove its identity to the InfluxDB server.

!!! Note
The certificate file needs to be copied to SD card folder `/config/certs`.<br>
    Supported formats:<br>
    - `PEM` (Base64-ASCII-coding, File extentions: `.pem, .crt, .cer, .key`)<br>
    - `DER` (Binary coding, File extention: `.der, .cer`)<br>
    Only unencrypted and not password protected files are supported.



!!! Note
    Using TLS for InfluxDB, adaptions of InfluxDB `URI` parameter needs to be done, as well.  Please ensure 
    protocol `https://` is configured, e.g. `https://IP-ADDRESS:8086`

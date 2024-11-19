# Parameter: Client Key

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Client Key          | clientkey
| Default Value     | empty               | empty


## Description

Select client private key file.<br>
Keep it empty if mutual authentication is not required. If configured, `Client Certificate` needs to be configured, too.

The client private key is used for TLS handshake of InfluxDB mutual authentification. The client certificate and 
related client private key is used by the HTTP client to prove its identity to the InfluxDB server.


!!! Note
The certificate file needs to be copied to SD card folder `/config/certs`.<br>
    Supported formats:<br>
    - `PEM` (Base64-ASCII-coding, File extentions: `.pem, .crt, .cer, .key`)<br>
    - `DER` (Binary coding, File extention: `.der, .cer`)<br>
    Only unencrypted and not password protected files are supported.

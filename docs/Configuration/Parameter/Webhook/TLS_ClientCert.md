# Parameter: Client Certificate

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Client Certificate  | clientcert
| Default Value     | None                | empty


## Description

Select client certificate file<br>
The client certificate and respective client key key is used by the client 
to prove its identity to the InfluxDB server (mutual authentification: mTLS). Set it 
to `None` if mutual authentication is not required. If configured, 
`Client Key` needs to be provided, as well.


!!! Note
    The certificate file needs to be copied to SD card folder `/config/certs`.<br>
    Supported formats:<br>
    - `PEM` (Base64-ASCII-coding, File extentions: `.pem, .crt, .cer`)<br>
    - `DER` (Binary coding, File extention: `.der, .cer`)<br>
    - Only unencrypted and not password protected files are supported.<br>
    - Only TLS v1.2 is supported<br>
    - Max. key length: 4096 Bit

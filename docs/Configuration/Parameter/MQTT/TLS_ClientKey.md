# Parameter: Client Key

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Client Key          | clientkey
| Default Value     | empty               | empty


## Description

Select client private key file<br>
The client key and respective client certificate is used by the MQTT client 
to prove its identity to the MQTT broker (mutual authentification: mTLS). Set it 
to `None` if mutual authentication is not required. If configured, 
`Client Certificate` needs to be provided, as well.


!!! Note
    The certificate file needs to be copied to SD card folder `/config/certs`.<br>
    Supported formats:<br>
    - `PEM` (Base64-ASCII-coding, File extentions: `.pem, .crt, .cer, .key`)<br>
    - `DER` (Binary coding, File extention: `.der, .cer`)<br>
    - Only unencrypted and not password protected files are supported.<br>
    - Only TLS v1.2 is supported<br>
    - Max. key length: 4096 Bit

# Parameter: CA Certificate

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | CA Certificate      | cacert
| Default Value     | Default (Built-In Certificate Bundle) | empty


## Description

Select CA (Certificate Authority) certificate<br>
The CA certificate is used by the client to check whether the broker (server) 
is the one it claims to be (TLS handshake).


!!! Note
    The certificate file needs to be copied to SD card folder `/config/certs`.<br>
    Supported formats:<br>
    - `PEM` (Base64-ASCII-coding, File extensions: `.pem, .crt, .cer`)<br>
    - `DER` (Binary coding, File extension: `.der, .cer`)<br>
    - Only unencrypted and not password protected files are supported.<br>
    - Only TLS v1.2 is supported<br>
    - Max. key length: 4096 Bit


!!! Tip
    If no certificate file is selected, built-in certificate bundle is used by default. 
    The bundle comes with a full list of root certificates from Mozilla's NSS root certificate store. 

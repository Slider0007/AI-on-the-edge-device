# Parameter: Client Certificate

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Client Certificate  | clientcert
| Default Value     | empty               | empty


## Description

Location of client certificate file (absolute path in relation to sdcard root)


The client certificate is used for TLS handshake for InfluxDB authentification. The client certificate and 
related client private key is used by the client to prove its identity to the server.

!!! Note
    The certificate file needs to be copied to SD card folder `/config/certs`.<br>
    Typical file extentions: `*.crt`, `*.pem`, `*.der`<br>
    Only unencrypted and not password protected files are supported.


!!! Note
    Using TLS for InfluxDB, adaptions of InfluxDB `URI` parameter needs to be done, as well.  Please ensure 
    protocol `https://` is configured, e.g. `https://IP-ADDRESS:8086`

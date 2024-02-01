# Parameter: Client Certificate

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Client Certificate  | TLSClientCert
| Default Value     | `/config/certs/client.crt` | `/config/certs/client.crt`


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!


## Description

Location of client certificate file (absolute path in relation to sdcard root)


The client certificate is used for TLS handshake for InfluxDB authentification. The client certificate and 
related client private key is used by the client to prove its identity to the server.

!!! Note
    Typical file extentions: `*.crt`, `*.pem`, `*.der`<br>
    Only unencrypted and not password protected files are supported.


!!! Note
    Using TLS for InfluxDB, adaptions of InfluxDB `URI` parameter needs to be done, as well.  Please ensure 
    protocol `https://` is configured, e.g. `https://IP-ADDRESS:8086`

# Parameter: Client Key

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Client Key          | TLSClientKey
| Default Value     | `/config/certs/client.key` | `/config/certs/client.key`


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!


## Description

Location of client private key file (absolute path in relation to sdcard root)<br>


The client private key is used for TLS handshake for InfluxDB authentification. The client certificate and 
related client private key is used by the client to prove its identity to the server.

!!! Note
    Typical file extention: `*.key`, `*.pem`<br>
    Only unencrypted and not password protected files are supported.


!!! Note
    Using TLS for InfluxDB, adaptions of InfluxDB `URI` parameter needs to be done, as well.  Please ensure 
    protocol `https://` is configured, e.g. `https://IP-ADDRESS:8086`

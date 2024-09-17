# Parameter: CA Certificate

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | CA Certificate      | cacert
| Default Value     | empty               | empty


## Description

Location of CA (Certificate Authority) certificate file (absolute path in relation to sdcard root)


The CA certificate is used for TLS handshake of InfluxDB authentification. The CA certificate is 
used by the client to validate the server is who it claims to be.


!!! Note
    The certificate file needs to be copied to SD card folder `/config/certs`.<br>
    Typical file extentions: `*.crt`, `*.pem`, `*.der`<br>
    Only unencrypted and not password protected files are supported.<br>

    
!!! Note
    Certificate CN field (common name) check is disabled by default (hard-coded).


!!! Note
    Using TLS for InfluxDB, adaptions of InfluxDB `URI` parameter needs to be done, as well. Please ensure 
    protocol `https://` is configured, e.g. `https://IP-ADDRESS:8086`

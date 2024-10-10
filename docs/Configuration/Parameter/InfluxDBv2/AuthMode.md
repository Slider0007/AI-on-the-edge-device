# Parameter: Authentication / Security

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Authentication / Security | authmode
| Default Value     | `Token`             | `1`
| Input Options     | `Token`<br>`TLS` | `1`<br>`2`


## Description

Select authentication mode for InfluxDB authentication.


| Input Option               | Description
|:---                        |:---
| `Token`                    | Authenticate with token
| `TLS`                      | Authenticate with token and TLS certificates


!!! Note
    The certificate files need to be copied to SD card folder `/config/certs` 
    and configured correctly.

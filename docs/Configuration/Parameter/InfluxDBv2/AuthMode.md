# Parameter: Authentication

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Authentication      | authmode
| Default Value     | `Basic`             | `1`
| Input Options     | `Basic`<br>`TLS` | `1`<br>`2`


## Description

Select authentication mode for InfluxDB authentication.


| Input Option               | Description
|:---                        |:---
| `Basic`                    | Authenticate with username and password
| `TLS`                      | Authenticate with username, password and TLS certificates


!!! Note
    The certificate files need to be copied to SD card folder `/config/certs` 
    and configured correctly.

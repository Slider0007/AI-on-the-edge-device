# Parameter: Authentication

|                   | WebUI               | Firmware
|:---               |:---                 |:----
| Parameter Name    | Authentication      | authmode
| Default Value     | `None`              | `0`
| Input Options     | `None`<br>`Basic`<br>`TLS` | `0` .. `2`


## Description

Select authentication mode for InfluxDB authentication.


| Input Option               | Description
|:---                        |:---
| `None`                     | No authentication, anonymous
| `Basic`                    | Authenticate with username and password
| `TLS`                      | Authenticate with username, password and TLS certificates


!!! Note
    The certificate files need to be copied to SD card folder `/config/certs` and configured correctly.

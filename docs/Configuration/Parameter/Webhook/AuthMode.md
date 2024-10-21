# Parameter: Authentication / Security

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Authentication / Security | authmode
| Default Value     | `None`              | `0`
| Input Options     | `None`<br>`Basic`<br>`TLS` | `0`<br>`1`<br>`2`


## Description

Select authentication / security mode for webhook authentication / security.


| Input Option               | Description
|:---                        |:---
| `None`                     | No authentication, anonymous
| `Basic`                    | Authenticate with username and password
| `TLS`                      | Authenticate with username, password and/or secure with TLS certificates


!!! Note
    The certificate files need to be copied to SD card folder `/config/certs` 
    and configured correctly.

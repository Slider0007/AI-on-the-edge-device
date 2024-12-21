# Parameter: Authentication

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Authentication      | authmode
| Default Value     | `Disabled`          | `0`
| Input Options     | `Disabled`<br>`Enabled` | `0`<br>`1`


## Description

Select authentication mode for WebUI / REST API authentication.


| Input Option               | Description
|:---                        |:---
| `Disabled`                 | WebUI / REST API endpoint authentication disabled, anonymous access allowed
| `Enabled`                  | WebUI / REST API endpoint authentication with username and password


!!! Warning
    Only basic access authentication scheme using unencrypted HTTP protocol is implemented. The basic authentication 
    scheme is not a secure method of user authentication, nor does it in any way protect the entity, which is transmitted 
    in cleartext (only Base64 encoded, not encyrpted or hashed) across the physical network used as the carrier. All data 
    (also security related data, e.g. password) are transmitted unencrypted (HTTP only, no HTTPS).


!!! Note
    The access is not only granted to WebUI, but also to any REST API endpoint, 
    because it's protected with same mechanism and linked to same account.

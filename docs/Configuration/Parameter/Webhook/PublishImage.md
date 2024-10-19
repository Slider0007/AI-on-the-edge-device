# Parameter: Publish Image

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Publish Image       | publishimage
| Default Value     | `Disabled`          | `0`
| Input Options     | `Disabled`<br>`Enabled`<br>`Enabled (On Error)` | `0`<br>`1`<br>`2`


## Description

Definition of publishing of actual image to the server data is published (using the same API Key). 


| Input Option               | Description
|:---                        |:---
| `Disabled`                 | Publishing image disabled
| `Enabled`                  | Publishing image enabled
| `Enabled (On Error)`       | Publishing image enabled, only if value status has error state `Rate too high >` or `Rate too high <`


!!! Note:
    HTTP method: `PUT`<br>
    Header:<br>
    - Content-Type: `application/json`
    - `APIKEY`: Parameter `API Key` Content



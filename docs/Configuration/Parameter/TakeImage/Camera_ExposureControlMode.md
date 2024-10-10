# Parameter: Exposure Control Mode

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Exposure Control Mode | exposurecontrolmode
| Default Value     | `Auto`              | `1`
| Input Options     | `Manual`<br>`Auto`<br>`Auto (AEC2)` | `0` .. `2`


## Description

Set exposure control mode


| Input Option  | Description
|:---           |:---
| `Manual`      | Manual exposure control. Exposure can be controlled by `Manual Exposure Value`.
| `Auto`        | Automatic exposure control. Exposure offset can be controlled by `Auto Exposure Level`.
| `Auto (AEC2)` | Automatic exposure control using alternative AEC2 Algo (extended range). Exposure offset can be controlled by `Auto Exposure Level`.


!!! Tip
    This parameter should be set on the 'Reference Image' configuration page. 
    There you have a visual feedback.

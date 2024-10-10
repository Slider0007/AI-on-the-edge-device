# Parameter: Gain Control Mode

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Gain Control Mode   | gaincontrolmode
| Default Value     | `Auto`              | `1`
| Input Options     | `Manual`<br>`Auto`  | `0` .. `1`


## Description

Set sensor gain control mode


| Input Option  | Description
|:---           |:---
| `Manual`      | Manual sensor gain control. Sensor gain can be controlled by `Manual Gain Value`.
| `Auto`        | Automatic sensor gain control. Automatic sensor gain adjustment up to `2x`.


!!! Tip
    This parameter should be set on the 'Reference Image' configuration page. 
    There you have a visual feedback.

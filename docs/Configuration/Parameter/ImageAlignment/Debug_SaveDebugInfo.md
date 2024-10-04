# Parameter: Save Debug Info

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Save Debug Info     | savedebuginfo
| Default Value     | `Disabled`          | `false`
| Input Options     | `Disabled`<br>`Enabled` | `false`<br>`true` 


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!


## Description

Save the following debug information in case of the following error during image alignment: <br>
1. Alignment algorithm failed to align image: Save alignment image with alignment marker <br>
<br>
Debug path: `/log/debug/`<br>

!!! Note
    This event will be additionally logged as an error event (process error). The actual 
    image evaluation processing going to be aborted. Only publishing states are still performed.


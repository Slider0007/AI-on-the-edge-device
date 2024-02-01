# Parameter: Save Debug Info

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Save Debug Info     | SaveDebugInfo
| Default Value     | `false`             | `false`
| Input Options     | `false`<br>`true`   | `false`<br>`true` 


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!


## Description

Save the following debug information in case of the following error during image alignment: <br>
1. Alignment algorithm failed to align image: Save alignment image with alignment marker <br>
<br>
Debug path: `/sdcard/log/debug/` <br>
<br>

!!! Note
    This event will be additionally logged as an error event (process error). The actual image evaluation processing 
    going to be aborted. Only publishing states are still performed.


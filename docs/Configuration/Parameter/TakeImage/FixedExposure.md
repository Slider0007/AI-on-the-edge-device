# Parameter: Fixed Exposure

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Fixed Exposure      | FixedExposure
| Default Value     | `false`             | `false`
| Input Options     | `false`<br>`true`   | `false`<br>`true` 


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!  


## Description

This option fixes the illumination setting of camera after intial capture and uses the same setting for any further capture event. This will slightly reduce processing time of 'Take Image' state but could result in worse results if environmental condition changed afterwards.

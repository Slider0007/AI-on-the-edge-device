# Parameter: Automatic Process Start

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Automatic Process Start | AutoStart
| Default Value     | `true`              | `true`
| Input Options     | `false`<br>`true`   | `false`<br>`true` 


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!


## Description

Start the process (digitization round) immediately after power up and restart in periodic interval (Processing Interval). 


!!! Note
    Typically this is set to `true`. If set to `false` process start can be triggered manually using the 
    [REST API](https://jomjol.github.io/AI-on-the-edge-device-docs/REST-API) or [MQTT-API](https://jomjol.github.io/AI-on-the-edge-device-docs/MQTT-API).

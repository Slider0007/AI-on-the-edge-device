# Parameter: Home Assistant Discovery

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Home Assistant Discovery | discoveryenabled
| Default Value     | `Disabled`          | `false`
| Input Options     | `Disabled`<br>`Enabled` | `false`<br>`true` 


## Description

Enable or disable the Home Assistant discovery. 
Refer to MQTT API - Home Assistant desciption for more details about Home Assistant discovery topics.

!!! Note
    - If enabled, discovery topics get automatically published once after a device restart.<br>
    - If enabled, discovery topics get automatically published after Home Assistant status topic changed to 'online'.<br>
    (Ensure Home Assistant status topic parameter is set correctly -> parameter `Home Assistant Status Topic`).<br>
    - This request can also be triggerd by `WebUI > Manual Control > Resend HA Discovery`.<br>
    - This request can also be triggered by REST API: `/mqtt?task=publish_ha_discovery`.<br>
    Check REST API description for more details.

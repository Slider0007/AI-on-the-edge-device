# Parameter: Homeassistant Discovery

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Homeassistant Discovery | HomeassistantDiscovery
| Default Value     | `false`             | `false`
| Input Options     | `false`<br>`true`   | `false`<br>`true` 


## Description

Enable or disable the Homeassistant Discovery.<br>
If activated, the discovery topics gets automatically scheduled to sent once after device startup during state "Publish to MQTT".
To schedule a retransmission: Use "Manual Control > Resend HA Discovery" or call REST API: 
<a href=mqtt_publish_discovery target="_blank">http://&lt;IP&gt;/mqtt_publish_discovery</a><br>
Check [MQTT API - HA description](../../../API/MQTT/homeassistant_discovery.md) for more details.

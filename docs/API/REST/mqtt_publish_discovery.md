[Overview](_OVERVIEW.md) 

## REST API endpoint: mqtt_publish_discovery

`http://IP-ADDRESS/mqtt_publish_discovery`


Trigger for a manual publish of Home Assistant MQTT discovery topics. The request will be executed in the next upcoming "Publish To MQTT" state.

!!! __Note__: <br>
    1. The Home Assistant Discovery function needs to be activated in configuration. Please check MQTT section in configuration.<br>
    2. If the function is activated, discovery topics gets automatically published once after a device restart in the above mentioned state.<br>
    3. This request can also be triggerd from `WebUI > Manual Control > Resend HA Discovery`


Payload:
- No payload needed

Response:
- Content type: `HTML`
- Content: Query response

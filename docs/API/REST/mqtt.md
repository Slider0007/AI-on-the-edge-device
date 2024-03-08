[Overview](_OVERVIEW.md) 

## REST API endpoint: mqtt

`http://IP-ADDRESS/mqtt`


MQTT related tasks

Payload:
  - `task` Task to perform
  - Available options:
    - `api_name` API name + version
      - Example: `/mqtt?task=api_name`
      - Response:
        - Content type: `HTML`
        - Content: `mqtt:vx`
    - `publish_ha_discovery` Publish Home Assistant discovery topics during 'Publish to MQTT' state<br>
        - !!! __Note__: <br>
            - Home Assistant discovery function needs to be enabled in configuration (MQTT section).<br>
            - If enabled, discovery topics get automatically published once after a device restart.<br>
            - If enabled, discovery topics get automatically published after Home Assistant status topic changed to 'online'.<br>
              (Ensure Home Assistant status topic parameter is set correctly.)
            - This request can also be triggerd by `WebUI > Manual Control > Resend HA Discovery`.
      - Example: `/camera?task=publish_ha_discovery`
      - Response:
        - Content type: `HTML`
        - Content: `001: Publication of HA discovery topics scheduled during state 'Publish to MQTT'`
    - `publish_device_info` Publish device info topics during 'Publish to MQTT' state
        - !!! __Note__: <br>
            - Device info topics (static content) get automatically published only once after a device restart.<br>
            - Device info topics are sent as retained messages.<br>
      - Example: `/camera?task=publish_device_info`
      - Response:
        - Content type: `HTML`
        - Content: `002: Publication of device info topics scheduled during state 'Publish to MQTT'`
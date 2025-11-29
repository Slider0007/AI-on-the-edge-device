[Overview](_OVERVIEW.md) 

## MQTT API: Device Control

The device can be controlled by publishing data to the following topics.

- Format: `[MainTopic]/device/ctrl/[Subscribed Topic]`
- Example: `watermeter/device/ctrl/reboot`

| Subscribed Topic            | Description                     | Payload
|:----------------------------|:--------------------------------|:--------------     
| `reboot`                    | Trigger a device reboot         | any character, length > 0

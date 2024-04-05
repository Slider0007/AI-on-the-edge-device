[Overview](_OVERVIEW.md) 

## MQTT API: GPIO

!!! __Note__: 
    The respective GPIO (General Purpose Input / Output) pin needs to be enabled in configuration 
    (`Settings` > `Configuration` > section `GPIO`).

### GPIO Control

GPIO pins can be controlled by publishing data to the following topic.

- Format: `[MainTopic]/device/gpio/[GPIOName]`
- Example: `watermeter/device/gpio/GPIO12`

| Topic                                | Description                 | Payload
|:-------------------------------------|:----------------------------|:--------------     
|`[MainTopic]/device/gpio/[GPIO Name]` | Control GPIO Pin State      | `HIGH`: `true` or `1`<br>`LOW`: `false` or `0`

---

### GPIO Status

GPIO pin status gets published to the following topic.

- Format: `[MainTopic]/device/gpio/[GPIOName]`
- Example: `watermeter/device/gpio/GPIO12`

| Topic                                | Description                 | Output
|:-------------------------------------|:----------------------------|:--------------     
| `[MainTopic]/device/gpio/[GPIOName]` | GPIO Pin State<br><br>Possible States:<br>- `HIGH`: `true`<br>- `LOW`: `false` | `false`

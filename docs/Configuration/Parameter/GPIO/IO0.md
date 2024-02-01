# Parameter group: GPIO0

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Group Name | GPIO0            | IO0
| Default State     | `disabled` -> unselected | `disabled` -> ;
| Default Value     | Configuration: `input`<br>Use Interrupt: `disabled`<br>PWM Duty Cycle Resolution: `10` Bit<br>Enable MQTT: `false`<br>Enable REST API: `false`<br>Name: ` `  | `input disabled 10 false false  `
| Input Options: Configuration | `input`<br>`input-pullup`<br>`input-pulldown`<br>`output` | `input`<br>`input-pullup`<br>`input-pulldown`<br>`output`
| Input Options: Use Interrupt | `disabled`<br>`rising edge`<br>`falling edge`<br>`rising and falling`<br>`low level trigger`<br>`high level trigger` | `disabled`<br>`rising-edge`<br>`falling-edge`<br>`rising-and-falling`<br>`low-level-trigger`<br>`high-level-trigger`
| Input Options: PWM Duty Cycle Resolution | `1` .. `20` Bit | `1` .. `20` Bit
| Input Options: Enable MQTT | `false`<br>`true` | `false`<br>`true`
| Input Options: Enable REST API | `false`<br>`true` | `false`<br>`true`
| Input Options: Name | Empty = `GPIO0`<br>Any name with `a-z, A-Z, 0-9, _, -`. | Empty = `GPIO0`<br>Any name with `a-z, A-Z, 0-9, _, -`.


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!


## Description

This parameter group can be used to configure the GPIO `IO0` pin.

!!! Warning
    This pin is only usable with some restrictions. It must be disabled when the camera module is used.
    Additionally, it is used to activate bootloader mode and must therefore be HIGH after a reset!


### Parameter

| Parameter                 | Description
|:---                       |:---
| Configuration             | Set the pin configuration
| Use Interrupt             | Enable interrupt for GPIO pin
| PWM Duty Cycle Resolution | Set PWM duty resolution for the internal LED controller
| Enable MQTT               | If enabled the pin sate is provided by MQTT and pin can be controlled by MQTT (output only)
| Enable REST API           | If enabled the pin sate is provided by REST API and pin can be controlled by REST API (output only)
| Name                      | Name which is used for this pin (MQTT topic, REST API)

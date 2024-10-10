# Parameter: Pin Mode

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Pin Mode            | pinmode
| Default Value     | `input-pullup`      | `input-pullup`
| Input Options     | `input`<br>`input-pullup`<br>`input-pulldown`<br>`output`<br>`output-pwm`<br>`flashlight-default`<br>`flashlight-pwm`<br>`flashlight-smartled`<br>`flashlight-digital`<br>`trigger-cycle-start` | `input`<br>`input-pullup`<br>`input-pulldown`<br>`output`<br>`output-pwm`<br>`flashlight-default`<br>`flashlight-pwm`<br>`flashlight-smartled`<br>`flashlight-digital`<br>`trigger-cycle-start`


## Description

GPIO operation mode


### Input Options

| Input Option              | Direction | Description
|:---                       |:---       |:---
| `input`                   | input     | Use as input, internal pullup and pulldown resistor is disabled
| `input pullup`            | input     | Use as input, internal pullup resistor is enabled
| `input pulldown`          | input     | Use as input, internal pulldown resistor is enabled
| `output`                  | output    | Use as output (digital states: 0, 1)<br>-> controlable by REST API and/or MQTT
| `output pwm`              | output    | Use as output which controlable by PWM duty (duty cycle: 0 .. Max duty resolution depending on PWM frequency)<br>-> controlable by REST API and/or MQTT
| `flashlight default`      | output    | This mode represents the board's default flashlight configuration, e.g. Default for board `ESP32CAM` -> GPIO4 as PWM controlled output. This mode is only visible on respective GPIO which is defined as default in firmware
| `flashlight pwm`          | output    | Use for flashlight operation with regular LEDs (PWM controlled intensity)
| `flashlight smartled`     | output    | Use for flashlight operation with smartLEDs
| `flashlight digital`      | output    | Use for flashlight operation with regular LEDs (digital states: 0, 1)
| `trigger cycle start`     | input<br>(pullup enabled) | Trigger a cycle start (by pulling signal to low)


!!! Note
    All flashlight modes are fully controlled by process cycle, no external manipulation allowed.


!!! Tip
    `Flashlight digital` / `Flashlight pwm` act like an output and are activated while 
    flashlight is requested by process (before image gets taken). This could potentially 
    be used to control any mechanism to activate display before image gets taken.
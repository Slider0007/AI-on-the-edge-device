# Parameter: Pin Mode

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Pin Mode            | pinmode
| Default Value     | `input-pullup`      | `input-pullup`
| Input Options     | `Input`<br>`Input Pullup`<br>`Input Pulldown`<br>`Output`<br>`Output PWM`<br>`Flashlight Default`<br>`Flashlight PWM`<br>`Flashlight Smartled`<br>`Flashlight Digital`<br>`Trigger Cycle Start`<br>`Resume WLAN Connection` | `input`<br>`input-pullup`<br>`input-pulldown`<br>`output`<br>`output-pwm`<br>`flashlight-default`<br>`flashlight-pwm`<br>`flashlight-smartled`<br>`flashlight-digital`<br>`trigger-cycle-start`<br>`resume-wlan-connection`


## Description

GPIO operation mode


### Input Options

| Input Option              | Direction | Description
|:---                       |:---       |:---
| `Input`                   | input     | Use as input, internal pullup and pulldown resistor is disabled
| `Input Pullup`            | input     | Use as input, internal pullup resistor is enabled
| `Input Pulldown`          | input     | Use as input, internal pulldown resistor is enabled
| `Output`                  | output    | Use as output (digital states: 0, 1)<br>-> controllable by REST API and/or MQTT
| `Output PWM`              | output    | Use as output which controllable by PWM duty (duty cycle: 0 .. Max duty resolution depending on PWM frequency)<br>-> controllable by REST API and/or MQTT
| `Flashlight Default`      | output    | This mode represents the board's default flashlight configuration, e.g. Default for board `ESP32CAM` -> GPIO4 as PWM controlled output. This mode is only visible on respective GPIO which is defined as default in firmware
| `Flashlight PWM`          | output    | Use for flashlight operation with regular LEDs (PWM controlled intensity)
| `Flashlight Smartled`     | output    | Use for flashlight operation with smartLEDs
| `Flashlight Digital`      | output    | Use for flashlight operation with regular LEDs (digital states: 0, 1)
| `Trigger Cycle Start`     | input<br>(pullup enabled) | Trigger a cycle start (by pulling signal to low)
| `Resume WLAN Connection`  | input<br>(pullup enabled) | Resume a suspended network connection (depending on network operation mode)

!!! Note
    All flashlight modes are fully controlled by process cycle, no external manipulation allowed.


!!! Tip
    `Flashlight Digital` / `Flashlight PWM` act like an output and are activated while 
    flashlight is requested by process (before image gets taken). This could potentially 
    be used to control any mechanism to activate display before image gets taken.
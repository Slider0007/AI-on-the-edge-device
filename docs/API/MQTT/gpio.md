[Overview](_OVERVIEW.md) 

## MQTT API: GPIO

!!! __Note__: 
    The respective GPIO (General Purpose Input / Output) pin needs to be enabled in configuration 
    (`Settings` > `Configuration` > section `GPIO` > `GPIO Expose to MQTT`).


### GPIO Control

GPIO pins can be controlled by publishing data to the following topic.

- Format: `[MainTopic]/device/gpio/[GPIOName]/ctrl`
- Example: `watermeter/device/gpio/gpio4/ctrl`

| Topic                                    | Description                 | Payload
|:-----------------------------------------|:----------------------------|:--------------     
|`[MainTopic]/device/gpio/[GPIOName]/ctrl` | Control GPIO Pin State<br><br>Valid for mode:<br>-`Output` | see Usage Details 1
|`[MainTopic]/device/gpio/[GPIOName]/ctrl` | Control GPIO Pin State<br><br>Valid for mode:<br>-`Output PWM` | see Usage Details 2


Usage Details
1. `[MainTopic]/device/gpio/[GPIOName]/ctrl`: Set GPIO output state (only applicaple for mode `Output`)<br>
    Payload (needs to be provided in JSON notation):
    - `state:` Desired state of GPIO output [`1` / `0`]
    - Example:
    ```
    {
        "state": 0
    }
    ```

2. `[MainTopic]/device/gpio/[GPIOName]/ctrl`: Set GPIO output state and PWM duty (only applicaple for mode `Output PWM`)<br>
    Payload (needs to be provided in JSON notation):
    - `state:` GPIO State [`1` or `0`]
    - `pwm_duty:` GPIO PWM Duty [0 .. 2^Duty Resolution - 1]
      - Duty Resolution is derived from configured PWM frequency, e.g. 5Khz frequency -> 13 Bit<br>
        - Formula: log2(APB CLK Frequency / Desired Frequency) = log2(80000000 / 5000) = 13.966<br>
        - Maximum resolution is limited to 14 Bit due to compability reasons (e.g. ESP32S3)
    - Example:
    ```
    {
        "state": 1,
        "pwm_duty": 1024
    }
    ```
---


### GPIO Status

GPIO pin status gets published to the following topic.

- Format: `[MainTopic]/device/gpio/[GPIOName]/state`
- Example: `watermeter/device/gpio/GPIO4/state`

| Topic                                      | Description                 | Output
|:-------------------------------------------|:----------------------------|:--------------     
| `[MainTopic]/device/gpio/[GPIOName]/state` | Actual GPIO Pin State<br><br>Valid for modes:<br>-`Input`<br>-`Input Pullup`<br>-`Input Pulldown`<br>-`Output`<br>-`Flashlight default`<br>-`Flashlight smartled`<br>-`Flashlight digital` | `{"state": 0}`
| `[MainTopic]/device/gpio/[GPIOName]/state` | Actual GPIO Pin State<br><br>Valid for modes:<br>-`Output PWM`<br>-`Flashlight PWM`<br>-`Flashlight default` | `{"state": 0, "pwm_duty": 1024}`


[Overview](_OVERVIEW.md) 

## REST API endpoint: GPIO

`http://IP-ADDRESS/gpio`


Interact with GPIO pins and retrieve GPIO configuration


Payload:
1. Control a GPIO output (GPIO mode: `Output`)
    - Format: `/gpio?task=set_state&gpio=[PinNumber]&state=1`
    - Example: `/gpio?task=set_state&gpio=4&state=0`
    - Response:
      - Content type: `HTML`
      - Content: `GPIO4, State: 0`

2. Control a GPIO PWM output (GPIO mode: `Output PWM`)
    - Format: `/gpio?task=set_state&gpio=[PinNumber]&state=1&pwm_duty=1024`
    - Example: `/gpio?task=set_state&gpio=4&state=0&pwm_duty=1024`
    - Response:
      - Content type: `HTML`
      - Content: `GPIO4, State: 0, PWM Duty: 1024`

3. Read a digital GPIO
    - Valid for GPIO modes
      - `Input`
      - `Input Pullup`
      - `Input Pulldown`
      - `Output`
      - `Flashlight Default` (depending on configuration)
      - `Flashlight Smartled`
      - `Flashlight Digital`
      - `Trigger Cycle Start`
    - Format: `/gpio?task=get_state&gpio=[PinNumber]`
    - Example: `/gpio?task=get_state&gpio=4`
    - Response:
      - Content type: `JSON`
      - Content: `{ "state": 1 }`

4. Read a PWM controlled GPIO
    - Valid for GPIO modes
      - `Output PWM`
      - `Flashlight PWM`
      - `Flashlight Default` (depending on configuration)
    - Format: `/gpio?task=get_state&gpio=[PinNumber]`
    - Example: `/gpio?task=get_state&gpio=4`
    - Response:
      - Content type: `JSON`
      - Content: `{ "state": 1, "pwm_duty": 1024 }`

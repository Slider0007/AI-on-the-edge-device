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

5. Get GPIO pin configuration
    - This is used to configure WebUI GPIO section based on board configuration defined in firmware (includes/defines.h)
    - Format: `/gpio?task=gpio_config`
    - Response:
      - Content type: `JSON`
      - Content: JSON response
      - Example: 
      ```
      {
          "default_config": {
              "gpio_modes": [
                  "input",
                  "input-pullup",
                  "input-pulldown",
                  "output",
                  "output-pwm",
                  "flashlight-pwm",
                  "flashlight-smartled",
                  "flashlight-digital",
                  "trigger-cycle-start"
              ],
              "gpio_interrupt": [
                  "cyclic-polling",
                  "interrupt-rising-edge",
                  "interrupt-falling-edge",
                  "interrupt-rising-falling"
              ]
          },
          "gpio": [
              {
                  "name": 1,
                  "usage": "restricted: uart0-tx"
              },
              {
                  "name": 3,
                  "usage": "restricted: uart0-rx"
              },
              {
                  "name": 4,
                  "usage": "flashlight-pwm"
              },
              {
                  "name": 12,
                  "usage": "spare"
              },
              {
                  "name": 13,
                  "usage": "spare"
              }
          ]
      }
      ```
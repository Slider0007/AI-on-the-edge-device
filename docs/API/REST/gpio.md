[Overview](_OVERVIEW.md) 

## REST API endpoint: GPIO

`http://IP-ADDRESS/gpio`


Interact with GPIO pins 


Payload:
1. Control a GPIO output
    - `/gpio?GPIO=[PinNumber]&Status=high`
    - `/gpio?GPIO=[PinNumber]&Status=low`
    - Example: `/gpio?GPIO=12&Status=high`

2. Read a GPIO input 
    - `/gpio?GPIO=[PinNumber]`
    - Example: `/gpio?GPIO=12`

Response:
- Content type: `HTML`
- Content: Query response text

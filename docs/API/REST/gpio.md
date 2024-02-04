[Overview](_OVERVIEW.md) 

### REST API endpoint: GPIO

`http://IP-ADDRESS/GPIO`


Interact with GPIO pins 


Payload:
1. Control a GPIO output
    - `/GPIO?GPIO=[PinNumber]&Status=high`
    - `/GPIO?GPIO=[PinNumber]&Status=low`
    - Example: `/GPIO?GPIO=12&Status=high`

2. Read a GPIO input 
    - `/GPIO?GPIO=[PinNumber]`
    - Example: `/GPIO?GPIO=12`

Response:
- Content type: `HTML`
- Content: Query response text
[Overview](_OVERVIEW.md) 

## REST API endpoint: set_fallbackvalue

`http://IP-ADDRESS/set_fallbackvalue`

Set the last valid value (fallback value) to given value or the actual RAW value


Payload:
1. Set to given value (value >= 0)
    - Example: `/set_fallbackvalue?numbers=main&value=1234.5678`  
      - `numbers`: Provide name of number sequence, e.g. `main`
      - `value`: provide the value to be set
    
2. Set to actual RAW value (value < 0, a valid RAW value is mandatory)
    - Example: `/set_fallbackvalue?numbers=main&value=-1`
      - `numbers`: Provide name of number sequence, e.g. `main`
      - `value`: Provide any negative number

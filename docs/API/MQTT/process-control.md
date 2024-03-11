[Overview](_OVERVIEW.md) 

## MQTT API: Process Control

The device can be controled by publishing data to the following topics.

- Format: `[MainTopic]/process/ctrl/[Subscribed Topic]`
- Example: `watermeter/process/ctrl/cycle_start`

| Subscribed Topic            | Description                     | Payload
|:----------------------------|:--------------------------------|:--------------     
| `cycle_start`               | Trigger a prcoess cyle start    | any character, length > 0
| `set_fallbackvalue`         | Set the last valid value (fallback value) to given value or the actual raw value | see Usage Details 1.

Usage Details
1. `[MainTopic]/process/ctrl/set_fallbackvalue`: Set the last valid value (fallback value) to given value or the actual raw value
    Payload (needs to be provided in JSON notation):
    - Set to given value (value >= 0)
      - `sequence:` Name of number sequence
      - `value:` Value to be set
      - Example:
        ```
        {
          "sequence": "main",
          "value": 1234.5678
        }
        ```
        
    - Set to actual raw value (value < 0; Precondition: Valid raw value is mandatory)
      - `"sequence":` Name of number sequence  
      - `"value":` Any negative number
      - Example:
        ```
        {
          "sequence": "main",
          "value": -1
        }
        ```



[Overview](_OVERVIEW.md) 

## REST API endpoint: wlan

`http://IP-ADDRESS/wlan`


WLAN related tasks

Payload:
  - `task` Task to perform
  - Available options:
    - `api_name` API name + version
      - Example: `/wlan?task=api_name`
      - Response:
        - Content type: `HTML`
        - Content: `wlan:vx`
    - `scan` Scan WLAN networks
      - Example: `/wlan?task=scan`
      - Response:
        - Content type: `JSON`
        - Content: Array of available WLAN networks
        - Example:
        ```
        {
            "networks": [
                {
                    "ssid": "SSID1",
                    "channel": 11,
                    "rssi": -52,
                    "authmode": "WPA2 WPA3 PSK"
                },
                                {
                    "ssid": "SSID2",
                    "channel": 1,
                    "rssi": -63,
                    "authmode": "WPA2 WPA3 PSK"
                },
                                {
                    "ssid": "SSID3",
                    "channel": 6,
                    "rssi": -69,
                    "authmode": "WPA2 WPA3 PSK"
                },
                                {
                    "ssid": "SSID4",
                    "channel": 1,
                    "rssi": -88,
                    "authmode": "WPA2 PSK"
                }
            ]
        }
        ```

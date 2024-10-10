[Overview](_OVERVIEW.md) 

## REST API endpoint: ota

`http://IP-ADDRESS/ota`


Perform an Over-The-Air (OTA) update


Payload:
- `task` Task
  - Available options:
    - `emptyfirmwaredir`
      - Delete all content in `/firmware`
      - No additional parameter necessary
    - `update`
      - Perform an update
      - Mandatory parameter: `file` 
    - `unziphtml`
      - Update WebUI of firmware only
      - Extracts the content of `/firmware/html.zip` to `/html`
      - No additional parameter necessary
- `file` Filename with extention but without path
  - Supported file extentions:
    - `TFLITE`: TFLite model
    - `TFL`: TFLite model (legacy)
    - `ZIP`: ZIP file (e.g. OTA release package)
    - `BIN`: MCU firmware (e.g. firmware.bin)
  - Note: File needs to be existing and located in folder `/firmware`
    
Example: `/ota?task=update&file=AI-on-the-edge-device__update__*.zip`


Response:
- Content type: `HTML`
- Content: Query response, e.g. `reboot`

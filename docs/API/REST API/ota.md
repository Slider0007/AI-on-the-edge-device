[Overview](_overview.md) 

### REST API endpoint: ota

`http://IP-ADDRESS/ota`


Perform an Over-The-Air (OTA) update

Payload:
  - `task` Task
    Available options:
    - `emptyfirmwaredir`
      - Delete all content of `/sdcard/firmware`
      - No additional parameter necessary
    - `update` Perform an update
      - Mandatory parameter: `file` 
    - `unziphtml`
      - Update WebUI of firmware only
      - Extracts the content of `/sdcard/firmware/html.zip` to `/sdcard/html`
  - `file` Filename with extention but without path
  BE AWARE: File needs to be existing and located in folder `/sdcard/firmware`
  Supported file extentions:
    - `TFLITE`: TFLite model
    - `TFL`: TFLite model (legacy)
    - `ZIP`: ZIP file (e.g. OTA release package)
    - `BIN`: MCU firmware (e.g. firmware.bin)
    
Example: `http://<IP>/ota?task=update&file=AI-on-the-edge-device__update__*.zip`


Response:
- Content type: `HTML`
- Content: Query response, e.g. `reboot`
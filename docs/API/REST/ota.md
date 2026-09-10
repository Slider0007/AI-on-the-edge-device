[Overview](_OVERVIEW.md) 

## REST API endpoint: ota

`http://IP-ADDRESS/ota`

Perform an Over-The-Air (OTA) update

The firmware or OTA package is sent directly as the `POST` request body. 
No query parameters or additional requests are required.

### Request

- **Method:** `POST`
- **Endpoint:** `/ota`
- **Parameters:** None
- **Body:**
  - OTA firmware package (Firmware image + WebUI assets)
  - MCU firmware image (ESP-format)

Supported file types:

| Extension | Description |
|---|---|
| `.ZIP` | OTA package containing firmware image and WebUI assets |
| `.BIN` | MCU firmware image |


The OTA handler automatically:

1. Receives the uploaded file
2. Determines the file type
3. Validates the uploaded content
4. Stages and processes the update
5. Safely extracts and promotes files
6. Installs firmware to the OTA partition
7. Performs firmware rollback validation (Pre-condition: latest bootloader, to be flashed manually)


### Response

- Content type: `text/html`
- Content:
  - On success: `success: Upload successful. Device reboots to process OTA package`
  - On failure: An appropriate HTTP error response is returned

This page lists possible blink codes of the onboard status LED, their meaning and possible solutions.

The error code source definition can be found [here](https://github.com/Slider0007/AI-on-the-edge-device/blob/develop/code/components/jomjol_helper/statusled.h).

# Design

  * 250ms blink code to identify source
  * 500ms defined LED off
  * 250ms blink code to identify error / status code
  * 1,5s defined LED off to signal repetition
  * Repetition blink code: infinite for critical errors and status indication or 2x for warning indication
  * e.g. 3x blinks | 500ms LED off | 2x blinks --> error: SD card not found


# Overview

| **source**    | source<br>blink count| error / warning / status              | status<br>blink count| repeat<br>infinite |
| ------------- | -------------------- |---------------------------------------| -------------------- | -------------------|
| WLAN_CONN     | 1                    | Disconnected (No Access Point)        | 1                    |
| WLAN_CONN     | 1                    | Disconnected (Authentication failure) | 2                    |
| WLAN_CONN     | 1                    | Disconnected (Timeout)                | 3                    |
| WLAN_CONN     | 1                    | Disconnected (further reasons)        | 4                    |  
| WLAN_INIT     | 2                    | WIFI init error (details console)     | 1                    | X
| SDCARD_NVS_INIT | 3                  | SD card filesystem mount failed       | 1                    | X
| SDCARD_NVS_INIT | 3                  | SD card not found (0x107)             | 2                    | X
| SDCARD_NVS_INIT | 3                  | SD card init failed (details console) | 3                    | X
| SDCARD_NVS_INIT | 3                  | NVS init failed: No partition found   | 4                    | X
| SDCARD_NVS_INIT | 3                  | NVS init failed: No free pages found  | 5                    | X
| SDCARD_NVS_INIT | 3                  | NVS init failed (details console)     | 6                    | X
| SDCARD_CHECK  | 4                    | Basic check: file creation/write error| 1                    | X
| SDCARD_CHECK  | 4                    | Basic check: file read/CRC error      | 2                    | X
| SDCARD_CHECK  | 4                    | Basic check: file delete error        | 3                    | X
| SDCARD_CHECK  | 4                    | Basic check: folder/file presence     | 4                    | X
| CAM_INIT      | 5                    | Initial camera init failed            | 1                    | 
| PSRAM_INIT    | 6                    | RAM init failed: Not found/defective  | 1                    | X
| PSRAM_INIT    | 6                    | External SPI RAM < 4MB                | 2                    | X
| PSRAM_INIT    | 6                    | Total heap < 4MB                      | 3                    | X
| TIME_CHECK    | 7                    | Missing time sync (check every round) | 1                    |
| OTA_OR_AP     | 8                    | OTA process ongoing                   | 1                    | X
| OTA_OR_AP     | 8                    | Soft AP started (for remote config)   | 2                    | X
| FLASHLIGHT    | N/A                  | LED on when flashlight is on          | solid, <br> no blink | 



# Errors / Warnings

## Source WLAN_CONN: WLAN disconnected

!!! __NOTE__:
    Only warning indication, blink code repetition: 2x
    --> General info: [WLAN disconnect reason code description](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html#wi-fi-reason-code)

### `WLAN disconnected (No Access Point)`
WLAN connection is interrupted due to no access point in range.

### `WLAN Disconnected (Authentication failure)`
WLAN connection is interrupted due to an authentication failure. If error repeats check WLAN config in WLAN.INI (username, password)

### `WLAN Disconnected (Timeout)`
WLAN connection is interrupted due to an timeout because no beacon from AP is received in a timely manner. Most probably access point  is not available anymore or connection is not reliable.

### `WLAN Disconnected (Further reasons)`
WLAN connection is interrupted due to further reasons. Disconnect reason is printed in warining message. Please check serial console output or logfile from sd card (using another device to retrieve logfile /log/message/). Please refer to this page to have additional infos in terms of WLAN disconnect reasons --> [WLAN disconnect reason code description](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html#wi-fi-reason-code)



## Source WLAN_INIT: WLAN initialization

!!! __NOTE__:
    All critical errors, regular boot not possible

### `SSID empty`
The mandatory parameter SSID (name of WLAN network) is empty. Please verify and reconfigure in `/config/config.json` and try again.

### `WIFI init error (details console)`
A general WIFI initialization error occured. Please check serial console output or logfile from sd card (using another device to retrieve logfile /log/message/) 



## Source SDCARD_INIT: SD card initialization

!!! __NOTE__:
    All critical errors, regular boot not possible

### `SD card filesystem mount failed`
Failed to mount FAT filesystem on SD card. Check SD card filesystem (only FAT supported) or try another card. Possible further infos: Please check serial console output.

### `SD card not found (Error code 0x107)`
SD card init failed. Check if SD card is properly inserted into SD card slot or try another card. Possible further infos: Please check serial console output.

### `SD card init failed (details console)`
A general SD card initialization error occured. Please check serial console output.

### `NVS init failed: No partition found`
A general NVS initialization error occured. No parition for NVS found in partition table. Check parition table configuration `partitions.csv`

### `NVS init failed: No free pages found`
A general NVS initialization error occured. No free NVS pages found. Check NVS parition size.

### `NVS init failed (details console)`
A general NVS initialization error occured. Please check serial console output.



## Source SDCARD_CHECK: SD card basic check

!!! __NOTE__:
    All critical errors, normal boot not possible. Reduced WebUI is going to be loaded for further diagnostic possibilities or redo firmware update.

### `File creation / write error`    
A basic check of SD card is performed at boot. Failed to create the test file or writing content to the file failed. Most likely SD card is defective or not supported. Please check logs with log viewer in reduced web interface, serial console output or try another card.

Recommendation: Format or try another card

### `File read / CRC verfication error`
A basic check of SD card is performed at boot. Failed to read the test file or CRC of read back content failed. Most likely SD card is defective. Please check logs with log viewer in reduced web interface or serial console output for further error indication or try another card.

Recommendation: Format or try another card

### `File delete error`
A basic check of SD card is performed at boot. Failed to delelte the test file. Most likely SD card is defective. Please check logs with log viewer in reduced web interface or serial console output for further error indication or try another card.

Recommendation: Format or try another card

### `Folder / File presence failed`
A basic check of SD card is performed at boot. One or more menadatory folder / file are not found on SD card. Please check logs with log viewer in reduced web interface or serial console output for further error indication.

Recommendation: Repeat installation using AI-on-the-edge-device__update__*.zip



## Source CAM_INIT: Camera initialization
### `Initial camera init failed`

!!! __NOTE__:
    Only warning indication during boot sequence, blink code repetition: 2x

Failed to initialze camera during boot sequence. The firmware will to continue regular boot and try to reinit camera automatically. Further errors can occur. Please check logs with logfile viewer if processing is behaving irregular.

Recommendation: Check for proper electrical connection, whether camera model is supported and whether power supply is sufficient.



## Source PSRAM_INIT: External RAM (SPI RAM) initialization

!!! __NOTE__:
    A critical errors, normal boot not possible. Reduced WebUI is going to be loaded for further diagnostic possibilities or redo firmware update.
    
### `SPI RAM init failed: Not found/defective`   
External RAM (SPI RAM) initialization failed. Most likely external RAM not accessable or defective. Normal operation is not possible without having external RAM.

### `External SPI RAM < 4MB`
External RAM (SPI RAM) initialization successful, but external RAM size is too small. A size of >= 4MB is necessary to run this firmware. 

### `Total heap < 4MB`
Total available system memory (heap) is too small. A size of >= 4MB is necessary to run this firmware. 



## Source TIME_CHECK: Time synchronization
### `Missing time sync (check every round)`

!!! __NOTE__:
    Only warning indication, blink code repetition: 2x

If system is configured to be synced with a NTP server the sync status is checked after every round (in state: "Flow finished". An warning message is also printed to log). If the time is not synced after serveral rounds, please check for proper configuration.



# Status

!!! __NOTE__:
    All only status indication

## Source OTA_OR_AP: OTA Update / Access point mode

### `OTA process ongoing`
An OTA is performed right now. Please wait until OTA is completed. System is rebooting automatically. If system is not coming up, please check serial console output.

### `Soft AP started (for remote config)`
The built-in access point functionality is started to perform initial remote remote setup. Further description: [Installtion --> `Section Remote Setup using the built-in Access Point`](https://jomjol.github.io/AI-on-the-edge-device-docs/Installation/)



## Source FLASHLIGHT: Flashlight

### `LED on when flashlight is on`
The LED is solid on as long the flashlight is on. This feature has lower priority than the other LED codes. Whenever another code occurs this feature will be overrided.



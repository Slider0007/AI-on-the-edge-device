# Reduced WebUI (error indication and tracing)

Whenever a critical error occurs during boot process which avoids loading of regular processing and regular webinterface, a reduced webinterface gets loaded to have at least some visual feedback and the possibilitiy to figure out the root cause by browsing the logfiles or trigger another OTA update.

The error code(s) get printed with specific error codes. This page lists the possible error codes, their meaning and possible solutions.

Note: Error codes defined in source code: [Error Codes](https://github.com/Slider0007/AI-on-the-edge-device/blob/develop/code/components/jomjol_helper/system.h).


## Critical Errors
Those Errors make the normal operation of the device impossible.
Most likely they are caused by a hardware issue!

### `0x00000001` PSRAM bad
Your device most likely has no PSRAM at all or it is too small (needs to have at least 4 MBytes)!
See [Hardware Compatibility](../Hardware-Compatibility).

Usually the log shows something like this:
```
psram: PSRAM ID read error: 0xffffffff
cpu_start: Failed to init external RAM!
```

### `0x00000002` Heap too small
The firmware failed to allocate enough memory. This most likely is a consequential error of a bad PSRAM!

### `0x00000008` SD card basic check failed
One or more basic SD card checks failed.

The following checks are performed during boot sequence:

 - Write a file (sdcard/sdcheck.txt) to SD card with some generic text
 - Read the written file back
 - CRC verification
 - Delete the file

Detailed error indication (write, rerad or delete error) can be derived from blinking code of red board status LED. Please refer to [Status LED Blink Codes](StatusLED_BlinkCodes.md)

Recommendation: Reformat SD card and check again or try another SD card

### `0x00000010` SD folder or file presence check failed
One or more mandatory folders and/or files are missing on SD card.
To have early indication that SD card is potentially ready for operation, some folder and files, which are mandatory are presence checked. This is not a 100% check and a successful test does not mean everthing is OK.

The following folders / files get checked during boot sequence:

- /sdcard/config
- /sdcard/config/certs --> created automatically in firmware
- /sdcard/html
- /sdcard/demo --> created automatically in firmware
- /sdcard/firmware --> created automatically in firmware
- /sdcard/img_tmp --> created automatically in firmware
- /sdcard/log --> created automatically in firmware
- /sdcard/wlan.ini
- /sdcard/config/config.ini
- /sdcard/html/index.html
- /sdcard/html/ota_page.html
- /sdcard/html/log.html
- /sdcard/html/common.js
- /sdcard/html/version.txt

Note: This list might be outdated, see the source code for the latest implementation: [SDCardCheckFolderFilePresence()](https://github.com/slider0007/AI-on-the-edge-device/blob/develop/code/components/jomjol_helper/sdcard_check.cpp#L14)

Recommendation: Check logs and / or redo a Over-The-Air Update (OTA Update) to ensure proper SD card structure



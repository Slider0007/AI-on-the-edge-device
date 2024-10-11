## Build

### Checkout Github Repository
```
git clone https://github.com/Slider0007/AI-on-the-edge-device.git
cd AI-on-the-edge-device
git checkout develop
git submodule update --init
```

### Optional Step: Update Submodules
```
cd code/components/{submodule} (e.g. esp32-camera)
git checkout VERSION (e.g. HASH of latest build)
cd ../../ (go back to code level)
git submodule update --init
```

---
### Build and Flash with console

#### Compile (firmware only)
```
Github project root directory --> cd code
platformio run --environment esp32cam
```

Check `platformio.ini` to find out which environments are available.

#### Compile (with HTML parameter tooltips, API docs and file hashes)
```
cd code
platformio run --environment esp32cam-localbuild
```
Check `platformio.ini` to find out which environments are available.

#### Upload
```
pio run --target upload --upload-port /dev/ttyUSB0
```

Alternatively, UART device can be defined in `platformio.ini`, eg. `upload_port = /dev/ttyUSB0`

#### Monitor UART Log
```
pio device monitor -p /dev/ttyUSB0 -b 115200
```

---
### Build and Flash with Visual Code IDE

- Download and install VS Code
  - https://code.visualstudio.com/Download
- Install the VS Code platformIO IDE plugin
  - <img src="https://raw.githubusercontent.com/Slider0007/ai-on-the-edge-device/develop/images/platformio_plugin.jpg" width="200" align="middle">
  - Check for error messages, maybe you need add some python libraries or other dependencies manually
    - e.g. in my Ubuntu a python3-env was missing: `sudo apt-get install python3-venv`
- git clone this project
  - in Linux: 
    ```
    git clone https://github.com/Slider0007/AI-on-the-edge-device.git
    cd AI-on-the-edge-device
    git checkout develop
    git submodule update --init
    ```

- in VS code, open the `AI-on-the-edge-device/code` 
	- from terminal: `cd AI-on-the-edge-device/code`
- open a pio terminal (click on the terminal sign in the bottom menu bar)
- make sure you are in the `code` directory
- To build, type `platformio run --environment esp32cam`
- To build with parameter tooltips and file hashes, type `platformio run --environment esp32cam-localbuild`
  - or use the graphical interface:
    <img src="https://raw.githubusercontent.com/Slider0007/ai-on-the-edge-device/develop/images/platformio_build.jpg" width="200" align="middle">
  - the build artifacts are stored in  `code/.pio/build/`
- Connect the device and type `pio device monitor`. There you will see your device and can copy the name to the next instruction
- Add `upload_port = you_device_port` to the `platformio.ini` file
- Make sure a SD card with the proper contents is inserted and you have adapted the WLAN configuration in `config.json`
- `pio run --target erase` to erase the flash
- `pio run --target upload` this will upload the `bootloader.bin, partitions.bin,firmware.bin` from the `code/.pio/build/esp32cam/` folder. 
- `pio device monitor` to observe the logs via uart

---
## Debugging

### UART/Serial Log
```
pio device monitor -p /dev/ttyUSB0 -b 115200
```
### Application Log File
The device is logging lots of actions to SD card (`log/messages`). This log can be viewed using WebUI (`System > Log Viewer`) or directly by browsing the files on SD card. Verbosity is depended on log level which can be adapted in WebUI

### Dump File
After a software exception a dump log will be written to flash. Find further details to the core functionality [here](https://docs.espressif.com/projects/esp-idf/en/v5.3.1/esp32/api-guides/core_dump.html)

Configuration:
- Location: partition `coredump` (compare `partitions.csv`)
- Log Format: ELF
- Integrity Check: CRC32


You can view the dump log backtrace summary directly in the WebUI or you can download the complete dump file for further analysis. (`System > System Info > Section 'Build'`). The downloaded dump file name has to following syntax: `{firmware version}__{board_type}_coredump-elf.bin`

#### ESP-IDF provides a special tool to help to analyze the downloaded core dump file
- Install [esp-coredump](https://github.com/espressif/esp-coredump) --> e.g. Installation using VSCode Platformio console: `pip install esp-coredump`
- Download SOC specific [ROM ELF files](https://github.com/espressif/esp-rom-elfs) and extract the hardware specific ELF file for further usage
- Make sure to use the matching version of `tool-xtensa-esp-elf-gdb`. If you are using VSCode with Platformio IDE, this package is already installed 
in `<path>/.platformio/packages`.
- Generic usage: 
    ```
    esp-coredump info_corefile --gdb <path_to_gdb_bin> --rom-elf <soc_specific_rom_elf_file> --core-format raw --core <downloaded coredump file> <elf file of actual firmware>
    ```
- Example: 
    ```
    esp-coredump info_corefile --gdb <path to tool-xtensa-esp-elf-gdb/bin/xtensa-esp32-elf-gdb.exe> --rom-elf esp32_rev0_rom.elf --core-format raw --core firmware_ESP32CAM_coredump-elf.bin firmware.elf
    ```


## Build

### Checkout Github Repository
```
git clone https://github.com/Slider0007/AI-on-the-edge-device.git
cd AI-on-the-edge-device
git checkout develop
```

---
### Build and Flash with console

#### Compile firmware and HTML files
```
Browse to GitHub project root directory
cd code
platformio run --environment {environment name}
```

Check `platformio.ini` to find out which environments are available.

Notes:
  1. Compiled files are located in `/code/.pio/build/{environment name}`
  2. Zip file (firmware + HTML files) is located in GitHub root directory (same structure than official package)


#### Upload
```
pio run --target upload --upload-port /dev/ttyUSB0
```

Alternatively, UART device can be defined in `platformio.ini`, eg. `upload_port = /dev/ttyUSB0`

#### Monitor Serial / UART Log
```
pio device monitor -p /dev/ttyUSB0 -b 115200
```

---
### Build and Flash with Visual Code IDE

#### Installation
- Download and install VS Code
  - https://code.visualstudio.com/Download
- Install the VS Code platformIO IDE plugin
  - <img src="https://raw.githubusercontent.com/Slider0007/ai-on-the-edge-device/develop/images/platformio_plugin.jpg" width="200" align="middle">
  - Check for error messages, maybe you need add some python libraries or other dependencies manually
- Checkout Github repository
    ```
    git clone https://github.com/Slider0007/AI-on-the-edge-device.git
    cd AI-on-the-edge-device
    git checkout develop
    ```
#### Usage
- Browse folder `AI-on-the-edge-device/code` 
	- Using terminal: `cd AI-on-the-edge-device/code`
- Open a PIO terminal (click on the terminal sign in the bottom menu bar)
- Make sure you are in the `code` directory
- To build, type `platformio run --environment {environment name}`
  - Or use the graphical interface:
    <img src="https://raw.githubusercontent.com/Slider0007/ai-on-the-edge-device/develop/images/platformio_build.jpg" width="200" align="middle">
  - The build artifacts are stored in `code/.pio/build/{environment name}`
- Connect the device and type `pio device monitor`. There you will see your device and can copy the name to the next instruction
- Make sure a SD card with the proper contents is inserted and you have adapted the WLAN configuration in `config.json`
- `pio run --target erase` to erase the flash
- `pio run --target upload` this will upload the `bootloader.bin`, `partitions.bin` and `firmware.bin` from the `code/.pio/build/{environment name}/` folder. 
- `pio device monitor` to observe the logs via UART

---
## Debugging

### UART/Serial Log
##### Using platformio IDE
```
pio device monitor -p /dev/ttyUSB0 -b 115200
```
##### Using [Web Installer](https://slider0007.github.io/AI-on-the-edge-device/)
<img src="../images/webinstaller_console.jpg">


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

---
## Source Code Style Guide
| Type               | Style                | Example
|--------------------|----------------------|-----
| Classes            | Pascal Case          | `ClassName`
| Structs            | Pascal Case          | `StructName`
| Functions          | Camel Case           | `callFunction1`
| Variables          | Camel Case           | `testVariable1`
| Constants          | Screaming Snake Case | `#define DEFINITION_1`

## Automatic Source Code Formatting

### Configuration
#### Pre-Condition
- Development environment has an automatic formatter function (e.g. VSCode)
- Formatting rule file (`.clang-format`) needs to be available in project root folder
- Every developer needs to use defined formatting rules to avoid unnecessary style changes

#### VSCode development environment
- No extention necessary (Using VSCode default formatter which is able to handle clang format)
- The formatting is applied automatically whenever pasting or saving the file by adding the following content to project specific `settings.json` file located in project subfolder `.vscode`. 
  ```
  "editor.formatOnSave": false,
  "editor.formatOnPaste": false,
  "[cpp]": {
      "editor.formatOnSave": true,
      "editor.formatOnPaste": true
  }
  ```
- With this settings it only applies per project and is enabled only for the language C++ (cpp, h files), but could also be configured globally.
- Formatting exlusion: Formating of file `defines.h` is disabled (`// clang-format off`) to keep better readability (nested PPDirectives)

### Formating rules (clang-format)
- Formatting rule file: [.clang-format](../.clang-format)
- [Online Configurator](https://clang-format-configurator.site/)
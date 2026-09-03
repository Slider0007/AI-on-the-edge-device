# Developer Guide & Build Documentation

This document covers how to build, flash, debug, and configure your development environment for the **AI-on-the-edge-device** firmware.

## Table of Contents
- [Building & Flashing](#building--flashing)
  - [Clone Repository](#clone-repository)
  - [Option A: CLI / Terminal](#option-a-cli--terminal)
  - [Option B: Visual Studio Code IDE](#option-b-visual-studio-code-ide)
- [Debugging](#debugging)
  - [Serial / UART Logs](#serial--uart-logs)
  - [Application Log Files](#application-log-files)
  - [Analyzing Core Dumps](#analyzing-core-dumps)
- [Code Style & Formatting](#code-style--formatting)
  - [Naming Conventions](#naming-conventions)
  - [IntelliSense & Auto-Completion (`clangd`)](#intellisense--auto-completion-clangd)
  - [Code Formatting (`clang-format`)](#code-formatting-clang-format)


## Building & Flashing
### Clone Repository

To get started, clone the repository and switch to the `develop` branch:

```bash
git clone https://github.com/Slider0007/AI-on-the-edge-device.git
cd AI-on-the-edge-device
git checkout develop
```

### Option A: CLI / Terminal
#### Compile Firmware and WebUI
```bash
Browse to GitHub project root directory
cd code
platformio run --environment {environment name}
```

Check `platformio.ini` to find out which environments are available.

Notes:
  1. Compiled files are located in `/code/.pio/build/{environment name}`
  2. Zip file (firmware + HTML files) is located in GitHub root directory (same structure than official package)

#### Upload
```bash
pio run --target upload --upload-port /dev/ttyUSB0
```

Alternatively, UART device can be defined in `platformio.ini`, eg. `upload_port = /dev/ttyUSB0`

#### Monitor Serial / UART Log
```bash
pio device monitor -p /dev/ttyUSB0 -b 115200
```

### Option B: Visual Studio Code IDE
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


## Debugging
### Serial / UART Logs
##### Using platformio IDE
```
pio device monitor -p /dev/ttyUSB0 -b 115200
```
##### Using [Web Installer](https://slider0007.github.io/AI-on-the-edge-device/)
<img src="../images/webinstaller_console.jpg">


### Application Log File
The device is logging lots of actions to SD card (`log/messages`). This log can be viewed using WebUI (`System > Log Viewer`) or directly by browsing the files on SD card. Verbosity is depended on log level which can be adapted in WebUI

### Analyzing Core Dumps
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


## Code Style & Formatting
### Naming Convention
| Type               | Style                | Example
|--------------------|----------------------|-----
| Classes            | Pascal Case          | `ClassName`
| Structs            | Pascal Case          | `StructName`
| Functions          | Camel Case           | `callFunction1`
| Variables          | Camel Case           | `testVariable1`
| Constants          | Screaming Snake Case | `#define DEFINITION_1`

### IntelliSense & Auto-Completion (`clangd`)
For fast, accurate code navigation, autocomplete, and inline linting, `clangd` is recommended over Microsoft's standard C/C++ extension.

1. **Install clangd extension**
  - Install the **clangd** extension from the VS Code Marketplace.
  - Disable the default **C/C++** extension (`ms-vscode.cpptools`) to prevent IntelliSense conflicts.
2. **Configure VS Code Settings**
  - Add the following to `.vscode/settings.json` to instruct `clangd` to load the compilation database and avoid standard IntelliSense conflicts:
    ```json
    "clangd.arguments": [
      "-j=4",
      "--pretty",
      "--enable-config",
      "--background-index",
      "--limit-results=20",
      "--limit-references=50",
      "--completion-style=detailed",
      "--header-insertion=never",
      "--clang-tidy",
      "--pch-storage=disk",
      "--query-driver={ADAPT_PATH_TO}/.platformio/packages/toolchain-xtensa-esp-elf*/bin/*.exe,C:/Users/Markus/.platformio/packages/toolchain-riscv32-esp*/bin/*.exe",
      "--compile-commands-dir=${workspaceFolder}/code/.pio/esp32cam",
    ],
    "C_Cpp.intelliSenseEngine": "disabled"
    ```
3. **Compilation database**
  - It's mandatory to have compilation database available: `compile_commands.json`
  - PlatformIO is automatically generate the `compile_commands.json` file under `.pio/{environment}` during regular project compilation.
  - Compile at least one project and verify that parameter `"--compile-commands-dir=...` is pointing to the `.pio/{environment}` directory

4. **.clangd file**
  - Ensure `.clangd` is in root of `code` directory

5. **Indexing**
  - It takes quite a while until initial indexing is completed. Be patient!


### Code Formatting (`clang-format`)
#### Formating rules (clang-format)
- Formatting rule file: [.clang-format](.clang-format)
- [Online Configurator](https://clang-format-configurator.site/)

#### Configuration
##### Pre-Condition
- Development environment has an automatic formatter function (e.g. VSCode)
- Formatting rule file (`.clang-format`) and (`.clangd`) file needs to be available in project root folder
- Every developer needs to use defined formatting rules to avoid unnecessary style changes

##### VSCode
- No extention necessary (Using VSCode default formatter which is able to handle clang format)
- The formatting is applied automatically whenever pasting or saving the file by adding the following content to project specific `settings.json` file located in project subfolder `.vscode` or having it globally defined in workspace or user environment.
  ```json
  "editor.formatOnSave": false,
  "editor.formatOnPaste": false,
  "[cpp]": {
      "editor.formatOnSave": true,
      "editor.formatOnPaste": false
  }
  ```
- With this settings it only applies per project and is enabled only for the language C++ (cpp, h files), but could also be configured globally.
- Formatting exlusion: Formating of file `defines.h` is disabled (`// clang-format off`) to keep better readability (nested PPDirectives)
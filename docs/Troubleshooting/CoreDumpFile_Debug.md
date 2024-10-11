## Debugging
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


# Firmware Installation (XIAO-ESP32S3-Sense)

## Manual Firmware Installation / Update

### STEP 1: Installing the MCU Firmware

The MCU of the device must first be flashed via a USB or serial connection.  
- ⚠️ Use the contents of `AI-on-the-edge-device__{Board Type}__*.zip`.
- ⚠️ Ensure you are using the correct firmware package for your specific board type.

There are multiple ways to flash the microcontroller:
- [Espressif Flash Tool](https://docs.espressif.com/projects/esp-test-tools/en/latest/esp32/production_stage/tools/flash_download_tool.html)  
- [esptool (command-line tool)](https://docs.espressif.com/projects/esptool/en/latest/esp32/esptool/index.html)

#### Procedure
  1. **Enter bootloader mode**: Keep 'B' button pushed (IO0 pulled to GND) while reseting board
  2. **Flash firmware with flash tool**: The three firmware bin files needs to be flashed with correct flash offset
  
      | Filename          | Offset      | Description      |
      |:------------------|:------------|:-----------------|
      | bootloader.bin    | 0x0         | Bootloader       |
      | partitions.bin    | 0x8000      | Partition Table  |
      | firmware.bin      | 0x10000     | User Application |

---

### STEP 2: Preparing The SD Card

An SD card is required for device operation, as the internal memory is insufficient to store all necessary files.

- ⚠️ Use the same firmware package `AI-on-the-edge-device__{Board Type}__*.zip` for this step.  
- ⚠️ **Do not use source files directly from the repository** — not even for preparing the SD card. Only use files 
from official precompiled release packages or GitHub CI compiled test versions. Using unsupported files may result 
in limited or broken functionality.

#### Procedure
  1. Format SD card with FAT32 (Windows recommended. In MacOS formatted cards may not working properly)
  2. Copy the complete `config` and `html` folders from the firmware ZIP to the root directory of the SD card
  3. Copy the config template file `/config/template/config.json` to the `/config` folder<br>
     - If the device has already been booted, a full default config file will exist in `/config`. You can modify that instead.
     - Ensure proper JSON syntax. Invalid formatting will cause the user configuration to be rejected and default will be used.
  4. Configure the network connection:
     - Enter your Wi-Fi credentials and optionally configure network settings in wlan section (default: DHCP)
  5. Insert the SD card into the device and power it on
  6. Access the device using:
     - **Hostname**: `http://watermeter`  
     - **Hostname via mDNS**: `http://watermeter.local`  
     - Or the configured **IP address**



## Alternative Provisioning Options

### Initial Device Provisioning via Web Installer

You can provision your device using a browser-based interface (no manual flashing required): 
[Web Installer Guide](https://github.com/Slider0007/AI-on-the-edge-device/blob/develop/docs/Installation/DeviceProvisioning/WebInstaller.md)


### Firmware Update via OTA (Over-the-Air)

- ⚠️ This feature is available **only after initial setup** and when the WebUI is accessible
- ✅ The device (MCU + SD card content) will be updated automatically
- ✅ Existing configuration will remain untouched

To update the firmware via OTA:

1. Open the device’s web interface
2. Go to **System > OTA Update** page
3. Upload the latest firmware ZIP from the GitHub release page

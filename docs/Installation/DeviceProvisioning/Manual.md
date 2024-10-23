## Device Provisioning

### Manual Installation (MCU + SD Card)
#### Step 1: Installation Of MCU Part Of Firmware

Initially the MCU of the device has to be flashed via a USB / serial connection.<br>
Use content of `AI-on-the-edge-device__{Board Type}__*.zip`.

<b>IMPORTANT:</b> Make sure to use correct firmware package for your board type.

There are different ways to flash the microcontroller:
- [Espressif Flash Tool](https://www.espressif.com/sites/default/files/tools/flash_download_tool_3.9.5.zip)<br>
- [ESPtool (command-line tool)](https://docs.espressif.com/projects/esptool/en/latest/esp32/esptool/index.html)

Check readme file in firmware package and [jomjol documentation](https://jomjol.github.io/AI-on-the-edge-device-docs/Installation/#manual-flashing) for further details.


#### Step 2: Installation Of SD Card Content
A SD card is mandatory to operate the device because of internal device memory is insufficient to handle all necessary files. Therefore the SD card needs to be preloaded with some file content to be able to operate the device.<br>

⚠️ Make sure, SD card is formated properly (FAT or FAT32 file system).<br>

Use firmware package `AI-on-the-edge-device__{Board Type}__*.zip` for installation process.<br>
⚠️ **Please do not use the source files directly from the repository, not even for the preparation of the SD card!** Use only files related to official precompiled release packages or test versions. Otherwise, full functionality cannot be guaranteed.<br>

##### Option 1: Manual SD Card Installation
- Copy complete `config` and `html` folder of `AI-on-the-edge-device__{Board Type}__*.zip` to SD card root folder
- Copy file `config/template/config.json` to `config` folder
- Configure WLAN and credentials
- Insert SD-card to device and boot device

##### Option 2: Provisioning via Access Point
Further details can be found in [Access Point Provisioning Documentation](AccessPoint.md).
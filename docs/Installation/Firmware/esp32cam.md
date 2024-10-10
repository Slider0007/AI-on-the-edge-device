# Firmware Installation (ESP32CAM)

## Manual Firmware Installation / Manual Firmware Update

  ### 1. Flash microcontroller content (ESP32S3 based microcontroller)

   - Put your board into bootloader mode (IO0 grounded + board reset)
   - Three files to be flashed with correct flash offset

      | Filename          | Offset      | Description
      |:------------------|:------------|:------
      | bootloader.bin    | 0x1000      | Bootloader
      | partitions.bin    | 0x8000      | Partition definition
      | firmware.bin      | 0x10000     | User application
  
  - Use `Flash Download Tool` from Espressif or the console-based `esptool` to flash the files.
  Usage is described [here](https://jomjol.github.io/AI-on-the-edge-device-docs/Installation/#flashing-using-the-flash-tool-from-espressif-gui).


  ### 2. Prepare SD Card
  - Format SD card with FAT32 (Windows recommenend. In MacOS formated cards could not working properly)
  - Copy folders `/config` and `/html` from zip file to SD card root folder


  ### 3. Configure WLAN

  #### Option 1: Modify content directly in config file
  Use config.json template from `sdcard/config/template` folder, copy it to 
  `sdcard/config` folder and to configure your wlan connection.<br>
  Note 1: If device is already booted once, a full config with default configuration is already sitting in `/config` folder. 
  You can also use this file, search for section network and adapt the wlan config there.
  Note 2: Be careful with syntax. Messing up the JSON notation, config is getting rejected.
  
  #### Option 2: Use Access Point feature
  Usage is decribed [here](https://jomjol.github.io/AI-on-the-edge-device-docs/Installation/#remote-setup-using-the-built-in-access-point).



## Web-based Firmware Installer

 --> Not supported up to now.



## OTA (Over-The-Air) Firmware Update

  Select firmware package zip from github repository in WebUI OTA Updater page. 
  The device is getting updated automatically. The existing configuration will not be overwritten.


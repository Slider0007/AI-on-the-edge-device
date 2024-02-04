# AI-on-the-Edge [SL Fork]
<img src="images/icon/watermeter.svg" width="80px"> 

Artificial intelligence based systems have become established in our everyday lives. Just think of speech or image recognition. Most of the systems rely on either powerful processors or a direct connection to the cloud for doing the calculations there. With the increasing power of modern processors, the AI systems are coming closer to the end user – which is usually called **edge computing**.
Here, this edge computing is put into a practically oriented example, where an AI network is implemented on an ESP32 device so: **AI on the Edge**.

This project allows you to digitize your **analog** water, gas, power and other meters using cheap and easily available hardware.

All you need is an [ESP32-CAM board](https://jomjol.github.io/AI-on-the-edge-device-docs/Hardware-Compatibility/) and something of a practical hand.

<img src="images/esp32-cam.png" width="120px">

## Key features
- Tensorflow Lite (TFLite) integration – including easy-to-use wrapper
- Inline image processing (feature detection, alignment, ROI extraction)
- **Small** and **cheap** device (3 x 4.5 x 2 cm³, < 10 EUR)
- Integrated camera and illumination
- Web interface for administration and control
- OTA interface for updating directly via web interface
- Full integration into [Home Assistant](docs/API/MQTT/homeassistant_discovery.md)
- InfluxDB v1.x + v2.x
- [MQTT v3.x](docs/API/MQTT/_OVERVIEW.md)
- [REST API](docs/API/REST/_OVERVIEW.md)


## Workflow
The device takes an image of your meter at a defined interval. It extracts the Regions of Interest (ROIs) from the image and runs them through artificial intelligence. 
As a result, you get the digitized value of your meter. There are several options for what to do with that value. Either send it to a MQTT broker, write it to InfluxDB or simply provide access to it via a REST API (JSON / HTML).

<img src="https://raw.githubusercontent.com/jomjol/AI-on-the-edge-device/master/images/idea.jpg" width="800"> 


## Impressions
### Hardware
<img src="https://raw.githubusercontent.com/jomjol/AI-on-the-edge-device/master/images/watermeter_all.jpg" width="266"><img src="https://raw.githubusercontent.com/jomjol/AI-on-the-edge-device/master/images/main.jpg" width="266"><img src="https://raw.githubusercontent.com/jomjol/AI-on-the-edge-device/master/images/size.png" width="266"> 


### Software
<img src="https://github.com/Slider0007/AI-on-the-edge-device/assets/115730895/07938912-7438-467c-80ca-f1538b37f98c" width="800"> 


## Device Installation
### 1. Inform Yourself
There is growing [documentation](https://jomjol.github.io/AI-on-the-edge-device-docs/) which provides you with a lot of information. Head there to get a start, set it up and configure it.

A lot of people created useful Youtube videos which might help you getting started.

Here is a small selection:

- [youtube.com/watch?v=HKBofb1cnNc](https://www.youtube.com/watch?v=HKBofb1cnNc)
- [youtube.com/watch?v=yyf0ORNLCk4](https://www.youtube.com/watch?v=yyf0ORNLCk4)
- [youtube.com/watch?v=XxmTubGek6M](https://www.youtube.com/watch?v=XxmTubGek6M)
- [youtube.com/watch?v=mDIJEyElkAU](https://www.youtube.com/watch?v=mDIJEyElkAU)
- [youtube.com/watch?v=SssiPkyKVVs](https://www.youtube.com/watch?v=SssiPkyKVVs)
- [youtube.com/watch?v=MAHE_QyHZFQ](https://www.youtube.com/watch?v=MAHE_QyHZFQ)
- [youtube.com/watch?v=Uap_6bwtILQ](https://www.youtube.com/watch?v=Uap_6bwtILQ)

For further background information, head to [Neural Networks](https://www.heise.de/select/make/2021/6/2126410443385102621), 
[Training Neural Networks](https://www.heise.de/select/make/2022/1/2134114065999161585) and [Programming on the ESP32](https://www.heise.de/select/make/2022/2/2204010051597422030).

### 2. Download Firmware Package
Officially released firmware packages can be downloaded on [Releases Page](https://github.com/slider0007/AI-on-the-edge-device/releases).

### 3. Install MCU Part Of Firmware
Initially the MCU of the device has to be flashed via a direct USB connection. Further updates can be performed directly over the air (OTA). <br>
For initial installation, use content of `AI-on-the-edge-device__manual-setup__*.zip`.<br>
NOTE: OTA updates will be performed with `AI-on-the-edge-device__update__*.zip` package.

There are different ways to flash the microcontroller:
- [Espressif Flash Tool](https://www.espressif.com/sites/default/files/tools/flash_download_tool_3.9.5.zip)<br>
  ![image](https://github.com/Slider0007/AI-on-the-edge-device/assets/115730895/fb3d659f-3e21-49fd-9d84-7224994b7e28)
- [ESPtool (command-line tool)](https://docs.espressif.com/projects/esptool/en/latest/esp32/esptool/index.html)

See [documentation](https://jomjol.github.io/AI-on-the-edge-device-docs/Installation/) for more information.<br>
Note: Installation using web installer is not supported by this forked repo.

### 4. Install SD Card Content
The SD card can be setup using the device's local WLAN hotspot after the MCU firmware got successfully installed  (`AI-on-the-edge-device__remote-setup__*.zip`). See the 
[documentation](https://jomjol.github.io/AI-on-the-edge-device-docs/Installation/#remote-setup-using-the-built-in-access-point) for details. For this to work, the SD card must be FAT formated (which is the default on a new SD card).<br>
Alternatively the SD card still can be setup manually without using hotspot, see the [documentation](https://jomjol.github.io/AI-on-the-edge-device-docs/Installation/#3-sd-card) for details (`AI-on-the-edge-device__manual-setup__*.zip`).

:warning: Do not use github source files in any case, use only release related zip package. Otherwise functionality cannot be fully guaranteed!


## API Description
### REST API
See [REST API](docs/API/REST/_OVERVIEW.md)

### MQTT API
See [MQTT API](docs/API/MQTT/_OVERVIEW.md)


## Build Yourself
See [Build Instructions](code/README.md)


## Support
⚠️ This is a forked version of [jomjol´s great software](https://github.com/jomjol/AI-on-the-edge-device) which is intented to use for my personal purposes only.

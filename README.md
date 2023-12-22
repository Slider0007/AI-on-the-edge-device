# Welcome to the AI-on-the-Edge (SL Fork)
<img src="images/icon/watermeter.svg" width="100px"> 

Artificial intelligence based systems have become established in our everyday lives. Just think of speech or image recognition. Most of the systems rely on either powerful processors or a direct connection to the cloud for doing the calculations there. With the increasing power of modern processors, the AI systems are coming closer to the end user – which is usually called **edge computing**.
Here, this edge computing is put into a practically oriented example, where an AI network is implemented on an ESP32 device so: **AI on the Edge**.

This project allows you to digitize your **analog** water, gas, power and other meters using cheap and easily available hardware.

All you need is an [ESP32-CAM board](https://jomjol.github.io/AI-on-the-edge-device-docs/Hardware-Compatibility/) and something of a practical hand.

<img src="images/esp32-cam.png" width="200px">

## Key features
- Tensorflow Lite (TFlite) integration – including easy-to-use wrapper
- Inline image processing (feature detection, alignment, ROI extraction)
- **Small** and **cheap** device (3 x 4.5 x 2 cm³, < 10 EUR)
- Integrated camera and illumination
- Web interface for administration and control
- OTA interface for updating directly via the web interface
- Full integration into Home Assistant
- InfluxDB v1.x + v2.x
- MQTT v3.x
- REST API


## Workflow
The device takes a photo of your meter at a defined interval. It then extracts the Regions of Interest (ROIs) from the image and runs them through artificial intelligence. 
As a result, you get the digitized value of your meter. There are several options for what to do with that value. Either send it to an MQTT broker, write it to an InfluxDB 
or simply provide access to it via a REST API (JSON / HTML).

<img src="https://raw.githubusercontent.com/jomjol/AI-on-the-edge-device/master/images/idea.jpg" width="800"> 


## Impressions
### Hardware
<img src="https://raw.githubusercontent.com/jomjol/AI-on-the-edge-device/master/images/watermeter_all.jpg" width="266"><img src="https://raw.githubusercontent.com/jomjol/AI-on-the-edge-device/master/images/main.jpg" width="266"><img src="https://raw.githubusercontent.com/jomjol/AI-on-the-edge-device/master/images/size.png" width="266"> 


### Software
<img src="https://github.com/Slider0007/AI-on-the-edge-device/assets/115730895/07938912-7438-467c-80ca-f1538b37f98c" width="800"> 


## Device installation
### 1. INFORM YOURSELF
There is growing [documentation](https://jomjol.github.io/AI-on-the-edge-device-docs/) which provides you with a lot of information. Head there to get a start, set it up and configure it.

A lot of people created useful Youtube videos which might help you getting started.
Here a small selection:

- [youtube.com/watch?v=HKBofb1cnNc](https://www.youtube.com/watch?v=HKBofb1cnNc)
- [youtube.com/watch?v=yyf0ORNLCk4](https://www.youtube.com/watch?v=yyf0ORNLCk4)
- [youtube.com/watch?v=XxmTubGek6M](https://www.youtube.com/watch?v=XxmTubGek6M)
- [youtube.com/watch?v=mDIJEyElkAU](https://www.youtube.com/watch?v=mDIJEyElkAU)
- [youtube.com/watch?v=SssiPkyKVVs](https://www.youtube.com/watch?v=SssiPkyKVVs)
- [youtube.com/watch?v=MAHE_QyHZFQ](https://www.youtube.com/watch?v=MAHE_QyHZFQ)
- [youtube.com/watch?v=Uap_6bwtILQ](https://www.youtube.com/watch?v=Uap_6bwtILQ)

For further background information, head to [Neural Networks](https://www.heise.de/select/make/2021/6/2126410443385102621), 
[Training Neural Networks](https://www.heise.de/select/make/2022/1/2134114065999161585) and [Programming on the ESP32](https://www.heise.de/select/make/2022/2/2204010051597422030).

### 2. DOWNLAOD FIRMWARE
Officially released firmware packages can be downloaded on [Releases Rage](https://github.com/slider0007/AI-on-the-edge-device/releases).<br>

### 3. INSTALL MCU FIMRWARE
Initially the device have to be flashed via a USB connection. Further updates can be performed directly over the air (OTA). <br>
For manual initial installation, use content of `AI-on-the-edge-device__manual-setup__*.zip`.<br>
NOTE: OTA updates will be performed with `AI-on-the-edge-device__update__*.zip` package.

There are different possibilities:
- [Espressif Flash Tool](https://www.espressif.com/sites/default/files/tools/flash_download_tool_3.9.5.zip)<br>
  ![image](https://github.com/Slider0007/AI-on-the-edge-device/assets/115730895/fb3d659f-3e21-49fd-9d84-7224994b7e28)
- [ESPtool (command-line tool)](https://docs.espressif.com/projects/esptool/en/latest/esp32/esptool/index.html)

See the [documentation](https://jomjol.github.io/AI-on-the-edge-device-docs/Installation/) for more information.

### 4. INSTALL SD CARD
The SD card can be setup using local WLAN hotspot after the MCU firmware got installed (`AI-on-the-edge-device__remote-setup__*.zip`). See the 
[documentation](https://jomjol.github.io/AI-on-the-edge-device-docs/Installation/#remote-setup-using-the-built-in-access-point) for details. For this to work, the SD card must be FAT formated (which is the default on a new SD card).<br>
Alternatively the SD card still can be setup manually, see the [documentation](https://jomjol.github.io/AI-on-the-edge-device-docs/Installation/#3-sd-card) for details (`AI-on-the-edge-device__manual-setup__*.zip`).

:warning: !!! Do not separate download github source files, use only release related zip package. Otherwise full functionality cannot be guaranteed !!!


## Changes and History 
[Changelog](CHANGELOG.md)<br>
(Forked from https://github.com/jomjol/AI-on-the-edge-device)


## Build It Yourself
See [Build Instructions](code/README.md)


## Additional Ideas & Community Support
Features can be posted in jomjol repo [issues](https://github.com/jomjol/AI-on-the-edge-device/issues).<br>
If you have any technical problems please search the jomjol repo [discussions](https://github.com/jomjol/AI-on-the-edge-device/discussions) and jomjol repo [open issues](https://github.com/jomjol/AI-on-the-edge-device/issues).

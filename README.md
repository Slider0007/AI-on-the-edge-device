# AI-on-the-Edge Device [SLFork]
<img src="images/icon/watermeter.svg" width="80px"> 

Artificial intelligence is everywhere, from speech to image recognition. While most AI systems rely on powerful processors or cloud computing, **edge computing** brings AI closer to the end user by utilizing the capabilities of modern processors.  
This project demonstrates edge computing using a low-cost, AI-capable Espressif SOC device (e.g. **ESP32**), to digitize your analog meters — whether water, gas or electricity. With affordable hardware and simple instructions, you can turn any standard meter into a smart device.

Let's explore how to make **AI on the Edge** a reality!


## Key features
- Tensorflow Lite (TFLite) integration – including easy-to-use wrapper
- Inline image processing (Image taking, Image alignment, ROI extraction, Post processing)
- Usage of **small** and **low-cost** AI-capable devices ([Supported Hardware](#supported-hardware))
- Integrated camera and illumination (depending on hardware capabilities)
- Web interface for visualization, control and administration 
- Over the air (OTA) firmware update via web interface


## APIs / Publishing Services / Home Automation Integrations
- Home Assistant Integration ([Home Assistant Discovery](docs/API/MQTT/home-assistant-discovery.md))
- [REST API](docs/API/REST/_OVERVIEW.md)
- [MQTT v3](docs/API/MQTT/_OVERVIEW.md)
- InfluxDB v1
- InfluxDB v2
- [Webhook Publishing](docs/API/Webhook/_OVERVIEW.md)
- [Prometheus/OpenMetrics Exporter](docs/API/Prometheus-OpenMetrics/_OVERVIEW.md)


## Workflow
The device takes an image of your meter at a defined interval. It extracts the Regions of Interest (ROIs) from the image and runs them through artificial intelligence. 
As a result, you get the digitized value of your meter. There are several options for what to do with that value. Either send it to a MQTT broker, write it to InfluxDB or simply provide access to it via a REST API (JSON / HTML).

<img src="images/idea.jpg" width="800"> 


## Impressions
### Hardware
<img src="images/watermeter_all.jpg" width="266"><img src="images/main.jpg" width="266"><img src="images/size.png" width="266"> 


### Web Interface
<img src="images/webinterface_overview.png" width="800"> 


## Supported Hardware
### Board
| Board Type                                                                     | SOC      | Firmware Release | Remarks                       
|:---                                                                            |:---      |:---           |:--- 
| [ESP32-CAM](http://www.ai-thinker.com/pro_view-24.html)                        | ESP32    | All           |⚠️ Only boards with $\ge$ 4MB RAM are supported<br>⚠️ Beware of inferior quality Chinese clones
| [XIAO ESP32 Sense](https://www.seeedstudio.com/XIAO-ESP32S3-Sense-p-5639.html) | ESP32S3  | $\ge$ v17.0.0 |⚠️ Running quite hot, a small heat sink is recommended<br>ℹ️ No onboard illumination: External illumination (PWM / SmartLED) required
| [Freenove ESP32S3-WROOM](https://github.com/Freenove/Freenove_ESP32_S3_WROOM_Board) | ESP32S3-WROOM-1-N16R8<br><br>ESP32S3-WROOM-1-N8R8 | $\ge$ v17.0.0<br><br>$\ge$ v17.1.0 |ℹ️ SOC and pin compatible boards with 8MB / 16MB flash and 8MB RAM are supported

### Camera
| Camera Type                                                                             | Sensor Resolution  | Digital Zoom | Firmware Release | Remarks                       
|:---                                                                                     |:---                |:---          |:---              |:--- 
| [OV2640](https://www.arducam.com/ov2640/)                                               | 2MP                | 1.0x - 2.5x  | All              | ℹ️ Officially EOL since 2009, but still very popular<br>ℹ️ Pin and function compatible Chinese clones are supported
| [OV5640](https://cdn.sparkfun.com/datasheets/Sensors/LightImaging/OV5640_datasheet.pdf) | 5MP                | 1.0x - 4.0x  | $\ge$ v17.0.0    |ℹ️ Officially EOL since 2019, but still very popular<br>ℹ️ Autofocus is not supported<br>ℹ️ Power consumption higher than OV2640<br>⚠️ Running quite hot, a small heat sink or a reduced camera frequency (10Mhz) is recommended<br>⚠️ ESP32-CAM: Camera functional. Deviation of core + I/O voltage supply (board: 1.2V / 3.3V, camera: 1.5V / 2.8V (abs. max. 4.5V)).<br>⚠️ XIAO ESP32S3 Sense: Camera functional. Small deviation of core voltage supply (board: 1.3V, camera: 1.5V)<br>⚠️ Freenove-ESP32S3-WROOM: Camera functional. Deviation of core voltage supply + I/O voltage supply (board: 1.2V / 3.3V, camera: 1.5V / 2.8V).

#### ⚠️ Important Note
The camera clock frequency (configurable via WebUI or config file) might have negative impact (interfere with WLAN signal) on wireless network responsiveness (slow loading WebUI, higher latency), especially using low quality boards or boards with onboard antenna. Depending on used hardware combination and WIFI channel, try to find the best camera clock frequency under the evaluation of network responsiveness and resulting image quality.


## Inform Yourself
There is growing [documentation](https://jomjol.github.io/AI-on-the-edge-device-docs/) which provides you with a lot of information. Head there to get a start, how to set it up and configure it.<br>
ℹ️ Not every description is 100% suitable for this fork. Therefore please check `docs` folder of this repository for any fork specific documentation.


## Firmware installation

There are multiple options to install the firmware and the SD card content.

### Download Firmware Package
Officially released firmware packages can be downloaded from [releases](https://github.com/slider0007/AI-on-the-edge-device/releases) page.<br>
A possibly already available development version (upcoming release version) can be previewed [here](https://github.com/Slider0007/AI-on-the-edge-device/pulls?q=is%3Aopen+is%3Apr+label%3A%22autorelease%3A+pending%22).

⚠️ **Please do not use the source files directly from the repository, not even for the preparation of the SD card!** Use only files related to the download sources mentioned here (official precompiled release packages or test versions). Otherwise, full functionality cannot be guaranteed.<br>

---
### Over The Air (OTA) Update
After the device is initially installed using one of the following installation options, it is **strongly recommended** to perform any further firmware update using the **web interface built-in OTA functionality**.

---
### Option 1: Web Installer (Only For Released Versions)

Follow the instructions listed at [Web Installer](https://slider0007.github.io/AI-on-the-edge-device/) page.<br>
Further details can be found in [Web Installer Provisioning Documentation](docs/Installation/DeviceProvisioning/WebInstaller.md).

<img src="images/webinstaller_home.jpg" width="800">

---
### Option 2: Manual Installation (MCU + SD Card)
Further details can be found in [Manual Provisioning Documentation](docs/Installation/DeviceProvisioning/Manual.md).


## API Description
### REST API
See [REST API Documentation](docs/API/REST/_OVERVIEW.md) in github repository or via device web interface (`System > Documentation > REST API`).<br>
ℹ️ Read API documentation carefully. REST API is not fully compatible with jomjol's original firmware.

### MQTT API
See [MQTT API Documentation](docs/API/MQTT/_OVERVIEW.md) in github repository or via device web interface (`System > Documentation > MQTT API`).<br>
ℹ️ Read API documentation carefully. MQTT API is not fully compatible with jomjol's original firmware.

### Prometheus Exporter
See [Prometheus API Documentation](docs/API/Prometheus-OpenMetrics/_OVERVIEW.md) in github repository or via device web interface (`System > Documentation > Prometheus API`).<br>
ℹ️ Read API documentation carefully. Prometheus API is not fully compatible with jomjol's original firmware.

### Webhook API
See [Webhook API Documentation](docs/API/Webhook/_OVERVIEW.md) in github repository or via device web interface (`System > Documentation > Webhook API`).<br>
ℹ️ Read API documentation carefully. Webhook API is not fully compatible with jomjol's original firmware.


## Build Yourself
See [Build / Debug Instructions](code/README.md)


## Support
ℹ️ This is a forked version of [jomjol´s great software](https://github.com/jomjol/AI-on-the-edge-device) which is intended to be used for my personal purposes only.

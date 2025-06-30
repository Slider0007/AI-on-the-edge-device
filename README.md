<h1>
  <img src="images/icon/watermeter.svg" width="40px" style="vertical-align: middle;">
  <span style="vertical-align: middle;">AI-on-the-Edge Device [SLFork]</span>
</h1>

Artificial intelligence is everywhere — from speech recognition to image analysis. While traditional AI often relies on powerful cloud 
servers, **AI-on-edge** runs directly on compact, affordable devices. This project brings edge computing to your home by using a low-cost, 
AI-enabled device to digitize your analog meters — whether it's water, gas or electricity. With budget-friendly hardware and an easy setup, 
you can turn any standard meter into a smart, connected device.

## Key Features
- **Compact & Cost-Effective** – Designed for small, affordable, AI-capable hardware
- **On-Device AI** – Integrated TensorFlow Lite (TFLite) enables efficient edge intelligence without external dependencies
- **Local Image Processing** – Fully processes and evaluates images on-device, with no need for cloud services
- **Web-Based Interface** – Browser UI for monitoring, configuration and control
- **OTA Update** – Seamless over-the-air firmware updates via the web interface

### ✨ Fork-Specific Enhancements
- **[Hardware](#supported-hardware)** – Compatible with ESP32 and ESP32-S3 devices
- **Connectivity** – Operates in WiFi Client or Access Point (Standalone) mode
- **Flashlight** – Customizable setup using multiple PWM-driven or smart LEDs or trigger an actuator
- **Durability** – Minimizes SD card wear by keeping process data (ROIs, models, markers) in RAM
- **Performance** - Reduced I/O cycles (data kept in RAM) and hardware-optimized routines
- **User Experience** – Consistent UI, dynamic config reloads without reboot, improved error handling/logging
- **Configuration** – Firmware-managed JSON-based config for better maintainability and resilience
- **Codebase** – Streamlined, mostly consistently styled, easier to maintain
- **API Design** – Relevant APIs use JSON notation for seamless integration
- **TLS Support** – Secure connections supported for MQTT, InfluxDB and Webhook

Further refinements are documented in the [changelog](CHANGELOG.md) (v16.0.0-SLFork and newer).


## APIs & Integrations
- **[Home Assistant Integration](docs/API/MQTT/home-assistant-discovery.md)** – Automatic device discovery via MQTT
- **[REST API](docs/API/REST/_OVERVIEW.md)** – Retrieve live data, check device status, and issue control commands over HTTP
- **[MQTT v3](docs/API/MQTT/_OVERVIEW.md)** – Publish data to your MQTT broker (TLS supported)
- **InfluxDB v1 / v2** – Log data directly into time-series databases (TLS supported)
- **[Webhook Publishing](docs/API/Webhook/_OVERVIEW.md)** – Push content to external services via HTTP hook (TLS supported)
- **[Prometheus/OpenMetrics Exporter](docs/API/Prometheus-OpenMetrics/_OVERVIEW.md)** – Export metrics for device monitoring purposes


Browse the links to explore the API documentation in this GitHub repository or through the device’s 
web interface: `System > Documentation`<br>
ℹ️ Please review the API documentation carefully. Note that the APIs are not fully compatible with jomjol’s original firmware.


## Workflow
The device captures an image of your meter at scheduled intervals and aligns it using predefined markers for accuracy. It then 
extracts the Regions of Interest (ROIs) from the image and processes these sections using AI. The extracted data is analyzed 
and converted into a digital reading, ready to be sent or accessed through various services and APIs (see above).

<img src="images/idea.jpg" width="800"> 


## Impressions
### Hardware
<img src="images/watermeter_all.jpg" width="266"><img src="images/main.jpg" width="266"><img src="images/size.png" width="266"> 


### Web Interface
<img src="images/webinterface_overview.png" width="800"> 


## Supported Hardware
### Board Compatibility
| Board Type | SOC / Module | Network Interfaces | Flashlight | Firmware Support | Remarks |
|:---|:---|:---|:---|:---|:---|
| [ESP32-CAM](http://www.ai-thinker.com/pro_view-24.html) | ESP32 | 1. WiFi Client<br>2. WiFi AP | ✅ Onboard LED | All | ⚠️ Only boards with ≥ 4MB RAM are supported<br>⚠️ Beware of inferior quality Chinese clones |
| [XIAO ESP32 Sense](https://www.seeedstudio.com/XIAO-ESP32S3-Sense-p-5639.html) | ESP32S3 | 1. WiFi Client<br>2. WiFi AP | ❌ External LED required (PWM, SmartLED) | ≥ v17.0.0 | ⚠️ Runs hot, small heatsink recommended |
| [Freenove ESP32S3-WROOM](https://github.com/Freenove/Freenove_ESP32_S3_WROOM_Board) | ESP32S3-WROOM-1-N16R8<br><br>ESP32S3-WROOM-1-N8R8 | 1. WiFi Client<br>2. WiFi AP | ✅ Onboard LED<br>Low intensity: Additional external LED recommended (PWM, SmartLED) | ≥ v17.0.0<br><br>≥ v17.1.0 | ℹ️ SOC and pin-compatible boards with 8/16MB flash and 8MB RAM supported |

### Camera Compatibility
| Camera Type | Sensor Resolution | Digital Zoom | Firmware Support | Remarks                       
|:---         |:---               |:---          |:---              |:---
| [OV2640](https://www.arducam.com/ov2640/) | 2MP | 1.0x - 2.5x | All | ℹ️ EOL since 2009, still widely used<br>ℹ️ Pin/function-compatible Chinese clones supported
| [OV5640](https://cdn.sparkfun.com/datasheets/Sensors/LightImaging/OV5640_datasheet.pdf) | 5MP | 1.0x - 4.0x | $\ge$ v17.0.0 |ℹ️ EOL since 2019, still widely used<br>ℹ️ Autofocus not supported<br>ℹ️ Power consumption higher than OV2640<br>⚠️ Tends to get hotter than OV2640 – use a heat sink or reduce camera frequency (default: 10MHz or lower)<br>⚠️ ESP32-CAM: Functional, but core / I/O voltage mismatch (board: 1.2V / 3.3V; camera: 1.5V / 2.8V (abs. max. 4.5V)).<br>⚠️ XIAO ESP32S3 Sense: Functional, minor core voltage deviation (board: 1.3V; camera: 1.5V)<br>⚠️ Freenove-ESP32S3-WROOM: Functional, but core / I/O voltage mismatch (board: 1.2V / 3.3V; camera: 1.5V / 2.8V).

#### ⚠️ Important Note
The camera clock frequency — configurable via the WebUI or config file — may negatively impact wireless network performance. This can 
result in slower WebUI loading times or increased latency, particularly when using low-quality boards or those with onboard antennas. 
To optimize performance, experiment with different camera clock frequencies while evaluating both network responsiveness and resulting 
image quality. The ideal setting may vary depending on your specific hardware setup and the Wi-Fi channel in use.


## Inform Yourself
There is growing [documentation](https://jomjol.github.io/AI-on-the-edge-device-docs/) which provides you with 
a lot of information. Head there to get a start, how to set it up and configure it.<br>
ℹ️ Not every description is 100% suitable for this fork. Therefore please check [docs](/docs/) folder of this repository 
for any fork specific documentation.


## Firmware Installation

There are several convenient options to install the firmware and prepare the SD card content.

### Download Firmware Builds
- **Releases** - Official firmware releases are available on the **[GitHub Releases Page](https://github.com/slider0007/AI-on-the-edge-device/releases)**<br>
- **Development Builds** - You can also test the latest development build / upcoming release via pull request labeled
[autorelease: pending](https://github.com/Slider0007/AI-on-the-edge-device/pulls?q=is%3Aopen+is%3Apr+label%3A%22autorelease%3A+pending%22). 
Follow the instructions at the bottom of the pull request to download the corresponding precompiled development build.

⚠️ **Important:** Do **not** use source files directly from the repository — this includes SD card preparation. Always use the official 
precompiled release packages or GitHub CI precompiled development builds. Using any of the source files may result in incomplete or 
non-functional firmware.

---
### Over The Air (OTA) Update
Once the initial installation is complete, it is **strongly recommended** to perform all future firmware updates via the **device’s 
web interface**: `System > OTA Update`. This method ensures seamless upgrades with minimal risk.

---
### Option 1: Web Installer (Only For Released Versions)

For the easiest and most user-friendly setup, use the **[Web Installer](https://slider0007.github.io/AI-on-the-edge-device/)**.<br>
Follow the step-by-step instructions on the Web Installer page. For more details, see the 
[Web Installer Provisioning Guide](docs/Installation/DeviceProvisioning/WebInstaller.md).

<img src="images/webinstaller_home.jpg" width="800">

---
### Option 2: Manual Installation (MCU + SD Card)
Follow the steps in the [Manual Provisioning Guide](docs/Installation/DeviceProvisioning/Manual.md) to flash the MCU and 
prepare the SD card manually.


## Build Yourself
Developers and advanced users can build the firmware from source. Follow the [Build / Debug Instructions](code/README.md) for environment setup
and compilation. If you don’t need to customize the firmware, it’s easier to use the precompiled releases provided on the 
[Releases page](https://github.com/slider0007/AI-on-the-edge-device/releases).


## Support
ℹ️ This is a forked version of [jomjol´s project](https://github.com/jomjol/AI-on-the-edge-device), customized for personal use. While community 
discussions, feedback and bug reports are welcome, please note that this fork is maintained independently and is **no longer compatible** with 
the upstream project. I keep my fork openly accessible to give something back to the community, even though I’m no longer actively working on the 
upstream project.

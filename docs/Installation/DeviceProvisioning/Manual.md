## Device Provisioning

### Manual Installation (MCU + SD Card)

#### Step 1: Installing the MCU Firmware

The MCU of the device must first be flashed via a USB or serial connection.  
Use the contents of `AI-on-the-edge-device__{Board Type}__*.zip`.

**IMPORTANT:** Ensure you are using the correct firmware package for your specific board type.

There are multiple ways to flash the microcontroller:
- [Espressif Flash Tool](https://docs.espressif.com/projects/esp-test-tools/en/latest/esp32/production_stage/tools/flash_download_tool.html)  
- [esptool (command-line tool)](https://docs.espressif.com/projects/esptool/en/latest/esp32/esptool/index.html)

Refer to the `README` file included in the firmware ZIP package for detailed instructions.

---

#### Step 2: Preparing the SD Card

An SD card is required for device operation, as the internal memory is insufficient to store all necessary files. 
The SD card must be preloaded with the correct content for the device to function properly.  

⚠️ **Ensure the SD card is properly formatted** using the FAT or FAT32 file system. macOS-formatted cards may cause issues.

ℹ️ Use the same firmware package `AI-on-the-edge-device__{Board Type}__*.zip` for this step.  

⚠️ **Do not use source files directly from the repository** — not even for preparing the SD card. Only use files 
from official precompiled release packages or GitHub CI compiled test versions. Using unsupported files may result 
in limited or broken functionality.

---

##### Option 1: Manual SD Card Setup

1. Copy the complete `config` and `html` folders from the firmware ZIP to the root directory of the SD card
2. Copy the file `/config/template/config.json` to the `/config` folder
3. Configure the network connection:
   - **Wi-Fi**: Enter your Wi-Fi credentials and optionally configure network settings (default: DHCP)
   - **Ethernet** (for devices with an ethernet port, default connection): Optionally configure network settings (default: DHCP)
4. Insert the SD card into the device and power it on
5. Access the device using:
   - Hostname: `http://watermeter`  
   - mDNS: `http://watermeter.local`  
   - Or the configured static IP address

---

##### Option 2: Semi-Automatic SD Card Setup

For semi-automated setup instructions, refer to the [SD Card Provisioning Documentation](SDCardProvisioning.md)

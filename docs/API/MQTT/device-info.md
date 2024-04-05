[Overview](_OVERVIEW.md) 

## MQTT API: Device Info

The following device status topics gets published during the `Publish To MQTT` state **only in the first cycle after boot**.

!!! __Tip__: 
    These topics can be manually published using REST API endpoint [mqtt](../../API/REST/mqtt.md).

!!! __Note__: 
    These topics are automatically set retained to ensure that they are available even after any 
    interruption of MQTT broker or any visualization software like Home Assistant or Openhab.


### Topics

- Format: `[MainTopic]/device/info/[Topic]`
- Example: `watermeter/device/info/network`


#### 1. Topic `[MainTopic]/device/info/hardware`

- Notation: JSON

| JSON Property               | Description                 | Output
|:----------------------------|:----------------------------|:--------------
| `board`.`board_type`        | Board Type                  | `ESP32CAM`  
| `board`.`chip_model`        | Device SOC Model            | `ESP32`
| `board`.`chip_cores`        | Device SOC Cores            | `2`
| `board`.`chip_revision`     | Device SOC Silicon Revision | `1.00`
| `board`.`chip_frequency`    | Device SOC CPU Frequency    | `160`
| `camera`.`type`             | Camera Type                 | `OV2640`
| `camera`.`frequency`        | Camera Frequency [Mhz]      | `20`
| `sdcard`.`capacity`         | SD card capacity [MB]       | `29580`
| `sdcard`.`partition_size`   | SD card partition size [MB] | `29560`


#### 2. Topic `[MainTopic]/device/info/network`

- Notation: JSON

| JSON Property               | Description                 | Output
|:----------------------------|:----------------------------|:--------------     
| `hostname`                  | Device Hostname             | `watermetter`
| `ipv4_address`              | Device IPv4 Address         | `192.168.1.x`
| `mac_address`               | Device MAC Address          | `44:21:D8:04:DF:A8`


#### 3. Topic `[MainTopic]/device/info/[Topic]`

- Notation: Single Topics

| Topic                       | Description                 | Output
|:----------------------------|:----------------------------|:--------------     
| `firmware_version`          | Firmware Version (MCU)      | `v17.0.0 (1234567)`



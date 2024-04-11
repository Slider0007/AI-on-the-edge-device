[Overview](_OVERVIEW.md) 

## REST API endpoint: info

`http://IP-ADDRESS/info`


Get process status and device info.<br>
Data is provided using JSON notation and also by HTML query request.

- JSON: `/info`
- HTML: `/info?type=xxx`

The following infos are available:

| JSON Property / HTML query `type=`   | Description                                        | Output
|:-------------------------------------|:---------------------------------------------------|:-----------------------
| `api_name`                           | Name of the API                                    | `info:v2`
| `process_status`                     | Process Status<br><br>Possible States:<br>- `Processing (Automatic)`:  Timer-controlled automatic processing<br>- `Processing (Triggered Only)`: Manual triggered processing only<br>- `Not Processing / Not Ready`: Initializing / Initialization failed | `Processing (Automatic)`
| `process_interval`                   | Process Interval [min]                             | `2.0`
| `process_time`                       | Process Time [sec]                                 | `25`
| `cycle_counter`                      | Process Cycle Counter                              | `64`
| `datalogging_sdcard_status`          | Datalogging Status<br><br>Possible states:<br>- `Enabled`<br>- `Disabled` | `Enabled`
| `mqtt_status`                        | MQTT Status<br><br>Possible states:<br>- `Connected (Encrypted)`<br>- `Connected`<br>- `Disconnected`<br>- `Disabled` | `Connected`
| `influxdbv1_status`                  | InfluxDB v1.x Status<br><br>Possible states:<br>- `Enabled (Encrypted)`<br>- `Enabled`<br>- `Disabled` | `Disabled`
| `influxdbv2_status`                  | InfluxDB v2.x Status<br><br>Possible states:<br>- `Enabled (Encrypted)`<br>- `Enabled`<br>- `Disabled` | `Disabled`
| `ntp_syncstatus`                     | NTP Synchronization Status<br><br>Possible states:<br>- `Synchronized`<br>- `In Progress`<br>- `Disabled` | `Synchronized`
| `current_time`                       | Current Date / Time                                | `2024-02-24T10:56:11+0100`
| `device_starttime`                   | Device Start / Boot Time                           | `2024-02-24T10:56:11+0100`
| `device_uptime`                      | Device Uptime [sec]                                | `496`
| `wlan_status`                        | WLAN Status<br><br>Possible states:<br>- `Connected`<br>- `Disconnected` | `Connected`
| `wlan_ssid`                          | WLAN SSID                                          | `IOTNETWORK`
| `wlan_rssi`                          | WLAN Signal Strength [dBm]                         | `-58`
| `mac_address`                        | Device MAC Address                                 | `41:23:D9:01:5E:AA`
| `network_config`                     | Network Configuration<br><br>Possible states:<br>- `DHCP`<br>- `Manual`| `DHCP`
| `ipv4_address`                       | Device IPv4 Address                                | `192.168.1.4`
| `netmask_address`                    | Device Netmask Address                             | `255.255.255.0`
| `gateway_address`                    | Device Gateway Address                             | `192.168.1.1`
| `dns_address`                        | Device DNS Address                                 | `192.168.1.1`
| `hostname`                           | Device Network Hostname                            | `watermeter`
| `board_type`                         | Board Type                                  | `ESP32CAM`
| `chip_model`                         | Device SOC Chip Model                              | `ESP32`
| `chip_cores`                         | Device SOC Chip Cores                              | `2`
| `chip_revision`                      | Device SOC Chip Revision                           | `1.0`
| `chip_frequency`                     | Device SOC Chip Frequency [Mhz]                    | `160`
| `chip_temp`                          | Device SOC Chip Temperature [°C]                   | `41`
| `camera_type`                        | Camera Type / Model                                | `OV2640`
| `camera_frequency`                   | Camera Clock Frequency [Mhz]                       | `20` 
| `sd_name`                            | SD Card Name                                       | `SD32G` 
| `sd_manufacturer`                    | SD Card Manufacturer                               | `Delkin/Phison (ID: 39)` 
| `sd_capacity`                        | SD Card Capacity [MB]                              | `29580` 
| `sd_sector_size`                     | SD Card Sector Size [Byte]                         | `512` 
| `sd_partition_alloc_size`            | SD Card Partition Allocation Size [Byte]           | `512` 
| `sd_partition_size`                  | SD Card Partition Size [MB]                        | `29560` 
| `sd_partition_free`                  | SD Card Partition Free Space [MB]                  | `29042` 
| `heap_total_free`                    | Heap Total Free [Byte]                             | `2479875` 
| `heap_internal_free`                 | Heap Internal Free [Byte]                          | `70719`
| `heap_internal_largest_free`         | Heap Internal Largest Block Free [Byte]            | `65536` 
| `heap_internal_min_free`             | Heap Internal Minimum Free [Byte]                  | `64247` 
| `heap_spiram_free`                   | Heap External SPIRAM Free [Byte]                   | `2409076` 
| `heap_spiram_largest_free`           | Heap External SPIRAM Largest Block Free [Byte]     | `2359296` 
| `heap_spiram_min_free`               | Heap External SPIRAM Minimum Free [Byte]           | `1359368` 
| `git_branch`                         | Git Branch Name                                    | `HEAD` 
| `git_tag`                            | Git Tag Name (`N/A` if no released version)        | `N/A` 
| `git_revision`                       | Git Revision / Commit                              | `3d7fed3` 
| `firmware_version`                   | MCU Firmware Version                               | `Develop: HEAD (Commit: 3d7fed3)` 
| `html_version`                       | WebUI / HTML Version                               | `Develop: HEAD (Commit: 3d7fed3)` 
| `build_time`                         | Firmware Build Time                                | `2024-01-23T21:32:23+0000` 
| `idf_version`                        | Espressif ESP IDF Development Framework Version    | `5.0.2` 

1. JSON:
    - Payload:
      - No payload needed
    - Response:
      - Content type: `JSON`
      - Content: JSON response
    - Example: 
```
{
    "api_name": "info:v2",
    "process_status": "Processing (Automatic)",
    "process_interval": 2.0,
    "process_time": 19,
    "cycle_counter": 24,
    "datalogging_sdcard_status": "Enabled",
    "mqtt_status": "Connected",
    "influxdbv1_status": "Disabled",
    "influxdbv2_status": "Disabled",
    "ntp_syncstatus": "Synchronized",
    "current_time": "2024-01-24T10:56:11+0100",
    "device_starttime": "2024-01-24T10:48:11+0100",
    "device_uptime": 496,
    "wlan_status": "Connected",
    "wlan_ssid": "IOTNET",
    "wlan_rssi": -58,
    "mac_address": "41:23:D9:01:5E:AA",
    "network_config": "Manual",
    "ipv4_address": "192.168.1.4",
    "netmask_address": "255.255.255.0",
    "gateway_address": "192.168.1.1",
    "dns_address": "192.168.1.1",
    "hostname": "watermeter",
    "board_type": "ESP32CAM",
    "chip_model": "ESP32",
    "chip_cores": 2,
    "chip_revision": "1.00",
    "chip_frequency": 160,
    "chip_temp": 41,
    "camera_type": "OV2640",
    "camera_frequency": 20,
    "sd_name": "SD32G",
    "sd_manufacturer": "Delkin/Phison (ID: 39)",
    "sd_capacity": 29580,
    "sd_sector_size": 512,
    "sd_partition_alloc_size": 512,
    "sd_partition_size": 29560,
    "sd_partition_free": 29042,
    "heap_total_free": 2479875,
    "heap_internal_free": 70719,
    "heap_internal_largest_free": 65536,
    "heap_internal_min_free": 64247,
    "heap_spiram_free": 2409076,
    "heap_spiram_largest_free": 2359296,
    "heap_spiram_min_free": 1359368,
    "git_branch": "HEAD",
    "git_tag": "N/A",
    "git_revision": "3d7fed6",
    "firmware_version": "Develop: HEAD (Commit: 3d7fed3)",
    "html_version": "Develop: HEAD (Commit: 3d7fed3)",
    "build_time": "2024-01-23T21:32:23+0000",
    "idf_version": "5.0.2"
}
```

2. HTML query request:
    - Payload:
      - `/info?type=___`
    - Response:
      - Content type: `HTML`
      - Content: HTML query response
    - Example: `/info?type=process_status` 

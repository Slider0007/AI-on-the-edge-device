[Overview](_OVERVIEW.md) 

## REST API Endpoint: `info`

`http://IP-ADDRESS/info`


Get process status and device info.<br>
Data is provided using JSON notation and also by HTML query request.

- JSON: `/info`
- HTML: `/info?type=xxx`

The following infos are available:

| JSON Property / HTML query `type=`   | Description                                        | Output
|:-------------------------------------|:---------------------------------------------------|:-----------------------
| `api_name`                           | Name of the API                                    | `info:v4`
| `process_status`                     | Process Status<br><br>Possible States:<br>- `Processing (Automatic)`:  Timer-controlled automatic processing<br>- `Processing (Triggered Only)`: Manual triggered processing only<br>- `Not Processing / Not Ready`: Initializing / Initialization failed | `Processing (Automatic)`
| `process_interval`                   | Process Interval [min]                             | `2.0`
| `process_time`                       | Process Time [sec]                                 | `25`
| `cycle_counter`                      | Process Cycle Counter                              | `64`
| `datalogging_sdcard_status`          | Datalogging Status<br><br>Possible states:<br>- `Enabled`<br>- `Disabled` | `Enabled`
| `mqtt_status`                        | MQTT Status<br><br>Possible states:<br>- `Connected (Encrypted)`<br>- `Connected`<br>- `Disconnected`<br>- `Disabled` | `Connected`
| `influxdbv1_status`                  | InfluxDB v1.x Status<br><br>Possible states:<br>- `Enabled (Encrypted)`<br>- `Enabled`<br>- `Disabled` | `Disabled`
| `influxdbv2_status`                  | InfluxDB v2.x Status<br><br>Possible states:<br>- `Enabled (Encrypted)`<br>- `Enabled`<br>- `Disabled` | `Disabled`
| `webhook_status`                     | Webhook Status<br><br>Possible states:<br>- `Enabled (Encrypted)`<br>- `Enabled`<br>- `Disabled` | `Disabled`
| `ntp_syncstatus`                     | NTP Synchronization Status<br><br>Possible states:<br>- `Synchronized`<br>- `In Progress`<br>- `Disabled` | `Synchronized`
| `current_time`                       | Current Date / Time                                | `2024-02-24T10:56:11+0100`
| `device_starttime`                   | Device Start / Boot Time                           | `2024-02-24T10:56:11+0100`
| `device_uptime`                      | Device Uptime [sec]                                | `496`
| `network_opmode`                     | Network Operation Mode<br><br>Possible states:<br>- `WLAN Client`<br>- `WLAN Access Point`<br>- `Ethernet`<br>- `Disabled` | `WLAN Client`
| `connection_status`                  | Connection Status<br><br>Possible states:<br>- `Connected`<br>- `Disconnected` | `Connected`
| `wlan_ssid`                          | WLAN SSID<br><br>Possible special value:<br>- `undefined`: WLAN disabled | `IOTNETWORK`
| `wlan_rssi`                          | WLAN Signal Strength [dBm]<br><br>Possible special values:<br>- `-127`: Not connected<br>- `-255`: WLAN disabled | `-58`
| `wlan_channel`                       | WLAN Channel<br><br>Possible special value:<br>- `-1`: WLAN disabled | `1` 
| `mac_address`                        | Device MAC Address                                 | `41:23:D9:01:5E:AA`
| `network_config`                     | Network Configuration<br><br>Possible states:<br>- `DHCP`<br>- `Static`| `DHCP`
| `ipv4_address`                       | Device IPv4 Address                                | `192.168.1.4`
| `netmask_address`                    | Device Netmask Address                             | `255.255.255.0`
| `gateway_address`                    | Device Gateway Address                             | `192.168.1.1`
| `dns_address`                        | Device DNS Address                                 | `192.168.1.1`
| `hostname`                           | Device Network Hostname                            | `watermeter`
| `board_type`                         | Board Type                                         | `ESP32CAM`
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
| `firmware_version`                   | MCU Firmware Version                               | `v17.4.1` 
| `html_version`                       | WebUI Version                                      | `v17.4.1` 
| `build_time`                         | MCU Firmware Build Time                            | `2024-01-23T21:32:23+0000` 
| `config_version`                     | Config Version                                     | `1` 
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
    "api_name": "info:vX",
    "process_status": "Processing (Automatic)",
    "process_interval": 2.0,
    ...
}
```

2. HTML query request:
    - Payload:
      - `/info?type=___`
    - Response:
      - Content type: `HTML`
      - Content: HTML query response
    - Example: `/info?type=process_status` 

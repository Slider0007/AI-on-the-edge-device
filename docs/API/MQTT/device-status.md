[Overview](_OVERVIEW.md) 

## MQTT API: Device Status

The following device status topics gets published during the `Publish To MQTT` state **every cycle**.

!!! __Note__: 
    These topics are not retained because they get updated on a regulary basis.

- Format: `[MainTopic]/device/status/[Topic]`
- Example: `watermeter/device/status/connection`

- Notation: JSON + Single Topics

Topic / [Topic].[JSON Property] | Description               | Output
|:----------------------------|:----------------------------|:--------------     
| `connection`                | MQTT Connection Status<br><br>Possible States:<br>- online<br>- offline | `online`
| `device_uptime`             | Device Uptime [s]           | `147`
| `wlan_rssi`                 | WLAN Signal Strength [dBm]  | `-54`
| `chip_temp`                 | Device CPU Temperature (°C) | `45`
| `heap`.`heap_total_free`    | Memory: Total Free (Int. + Ext.) [kB] | `3058639`
| `heap`.`heap_internal_free` | Memory: Internal Free [kB]  | `75079`
| `heap`.`heap_internal_largest_free` | Memory: Internal Largest Free Block [kB] | `65536`
| `heap`.`heap_internal_min_free`| Memory: Internal Minimum Free [kB] | `57647`
| `heap`.`heap_spiram_free`   | Memory: External Free [kB]  | `2409076`
| `heap`.`heap_spiram_largest_free`| Memory: External Largest Free Block [kB] | `2359296`
| `heap`.`heap_spiram_min_free`| Memory: External Minimum Free [kB] | `1359460`
| `sd_partition_free`         | SD Card: Free Partition Space | `29016`
| `ntp_sync_status`           | NTP Sync Status<br><br>Possible States:<br>- `Synchronized`<br>- `Not Synchronized`<br>- `Disabled` | `Synchronized`



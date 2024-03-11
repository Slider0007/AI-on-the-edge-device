[Overview](_OVERVIEW.md) 

## MQTT API: Home Assistant Discovery

### General

AI-on-the-Edge device support Home Assistant discovery.

1. Check [here](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery) to learn more about it and how to enable it in Home Assistant.
2. Enable it in the MQTT settings of your device. Make sure to select the right meter type to get the right units.
3. Home Assistant should pick up the topics automatically and show them under `Settings > Integrations > MQTT`.

Further information can be found in [jomjol documentation](https://jomjol.github.io/AI-on-the-edge-device-docs/Integration-Home-Assistant/).

### Topics

#### Sensor Class

The MQTT sensor integration is used to visualize device data / status.

- Format: `[HomeAssistantDiscoveryTopic]/sensor/[MainTopicWithoutStructure]/[DiscoveryTopic]/config`
- Example: `homeassistant/sensor/watermeter/uptime/config`

| Discovery Topic                        | Description                 
|:---------------------------------------|:----------------------------
`connection/config`                      | MQTT Connection Status
`device_uptime/config`                   | Device Uptime [s]
`wlan_rssi/config`                       | WLAN Signal Strength [dBm]
`heap_internal_free/config`              | Memory Internal Free [kB]
`heap_spiram_free/config`                | Memory External Free [kB]
`chip_temp/config`                       | Device CPU Temperature [°C]
`ntp_sync_status/config`                 | NTP Sync Status 
`ipv4_address/config`                    | Device IPv4 Address
`mac_address/config`                     | Device MAC Address
`hostname/config`                        | Device Hostname
`process_status/config`                  | Process Status
`process_interval/config`                | Process Interval [min]
`process_time/config`                    | Process Time [s]
`process_state/config`                   | Process State
`process_error_value/config`             | Process Error Value<br>-2: Three errors in row<br>-1: Error<br>0: OK<br>1: process deviation<br>2: Three process deviation in row
`[SequenceName]_actual_value/config`     | Actual value
`[SequenceName]_fallback_value/config`   | Fallback value<br>(Latest valid result)
`[SequenceName]_raw_value/config`        | Raw value
`[SequenceName]_value_status/config`     | Value Status
`[SequenceName]_rate_per_interval/config`| Rate per interval<br>(Delta of actual and last valid processed cycle)
`[SequenceName]_rate_per_time_unit/config` | Rate per HA time unit<br>(Delta of actual and last valid processed cycle + normalized to time unit, e.g. minute. The time unit gets derived from Home Assistant [meter type](../../Configuration/Parameter/MQTT/MeterType.md))
`[SequenceName]_timestamp_processed/config` | Timestamp of last processed cycle

Example: `ipv4_address/config`
```
{
  "~": "water/watermeterTest",
  "uniq_id": "watermeterTest_main_actual_value",
  "name": "main: Actual Value",
  "ic": "mdi:gauge",
  "stat_t": "~/process/data/1/json",
  "val_tpl": "{{value_json.actual_value}}",
  "qos": "1",
  "unit_of_meas": "m³",
  "dev_cla": "water",
  "stat_cla": "total_increasing",
  "avty_t": "~/device/status/connection",
  "dev": {
    "ids": [
      "watermeterTest"
    ]
  }
}
```


#### Binary Sensor Class

The MQTT binary sensor integration is used to indicate device errors / deviation in a row. This sensor gets 
only be triggerd when a device error or process deviation occured three times in a row.

- Format: `homeassistant/binary_sensor/[MainTopicWithoutStructure]/[DiscoveryTopic]/config`
- Example: `homeassistant/binary_sensor/watermeter/process_error/config`

| Discovery Topic                        | Description                 
|:---------------------------------------|:----------------------------
`process_error/config`                   | Process Error State (Problem > Three times device error [-2] / process deviation [2] in a row)

Example:
```
{
  "~": "water/watermeterTest",
  "uniq_id": "watermeterTest_process_error",
  "name": "Process Error State",
  "ic": "mdi:alert-outline",
  "stat_t": "~/process/status/process_error",
  "val_tpl": "{{ 'ON' if '-2' in value or '2' in value else 'OFF'}}",
  "qos": "1",
  "dev_cla": "problem",
  "avty_t": "~/device/status/connection",
  "dev": {
    "ids": [
      "watermeterTest"
    ]
  }
}
```


#### Button Class (Device Control)

The MQTT button integration is used to configure buttons for device control.

- Format: `homeassistant/button/[MainTopicWithoutStructure]/[DiscoveryTopic]/config`
- Example: `homeassistant/button/watermeter/process_error/config`

| Discovery Topic                        | Description                 
|:---------------------------------------|:----------------------------
`cycle_start/config`                     | Trigger a process cycle start

Example:
```
{
  "~": "water/watermeterTest",
  "uniq_id": "watermeterTest_cycle_start",
  "name": "Manual Cycle Start",
  "ic": "mdi:timer-play-outline",
  "cmd_t": "~/process/ctrl/cycle_start",
  "pl_prs": "1",
  "qos": "1",
  "dev_cla": "update",
  "ent_cat": "config",
  "avty_t": "~/device/status/connection",
  "dev": {
    "ids": [
      "watermeterTest"
    ]
  }
}

```



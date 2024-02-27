[Overview](_OVERVIEW.md) 

## MQTT API: Home Assistant Discovery

### General

AI-on-the-Edge devices support Home Assistant Discovery.

1. Check [here](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery) to learn more about it and how to enable it in Homeassistant.
2. Enable it in the MQTT settings of your device. Make sure to select the right meter type to get the right units.
3. Home Assistant should pick the topics up and show them under `Settings > Integrations > MQTT`.

Further information can be found in [jomjol documentation](https://jomjol.github.io/AI-on-the-edge-device-docs/Integration-Home-Assistant/).

### Topics

#### Sensor

- Format: `homeassistant/sensor/[hostname]/[Topic]/config`
- Example: `homeassistant/sensor/watermeter/uptime/config`

Discovery Topic Configuration| Description
:-|:-
`homeassistant/sensor/[hostname]/uptime/config` | Uptime
`homeassistant/sensor/[hostname]/IP/config` | Device IPv4 Address
`homeassistant/sensor/[hostname]/hostname/config` | Device Hostname
`homeassistant/sensor/[hostname]/interval/config` | Processing Interval [min]
`homeassistant/sensor/[hostname]/connection/config` | MQTT Connection Status
`homeassistant/sensor/[hostname]/uptime/config` | Device Uptime [s]
`homeassistant/sensor/[hostname]/freeMem/config` | Total Free Memory (Int. + Ext.) [kB]
`homeassistant/sensor/[hostname]/wifiRSSI/config` | WLAN Signal Strength [dBm]
`homeassistant/sensor/[hostname]/CPUtemp/config` | Device CPU Temperature (°C)
`homeassistant/sensor/[hostname]/status/config` | Process State
`[Main Topic]/[Sequence Name]/actual_value/config` | Actual value
`[Main Topic]/[Sequence Name]/fallback_value/config` | Fallback value<br>(Latest valid result)
`[Main Topic]/[Sequence Name]/raw_value/config` | Raw value <br>The value before performing any post-processing
`[Main Topic]/[Sequence Name]/value_status/config` | Value Status
`[Main Topic]/[Sequence Name]/rate_per_processing/config` | Rate per processing<br>(Delta of the last two valid processed cycles)
`[Main Topic]/[Sequence Name]/rate_per_time_unit/config` | Rate per HA time unit<br>(Delta of the last two valid processed cycles and normalized to time unit, e.g. minute. The time unit gets derived from Home Assistant [meter type](../../Configuration/Parameter/MQTT/MeterType.md))
`[Main Topic]/[Sequence Name]/timestamp_processed/config` | Timestamp of last processed cycle
`[Main Topic]/[Sequence Name]/json/config` | Provide the following content in JSON notation: `actual_value`, `fallback_vaue`, `raw_value`, `value_status`, `rate_per_min`, `rate_per_processing`, `timestamp_processed`

Example: `[Main Topic]/[Sequence Name]/actual_value/config`
```
{
  "~": "watermeterTest",
  "unique_id": "watermeter-actual_value",
  "object_id": "watermeter_actual_value",
  "name": "Actual Value",
  "icon": "mdi:gauge",
  "state_topic": "~/main/actual_value",
  "state_class": "total_increasing",
  "availability_topic": "~/connection",
  "payload_available": "connected",
  "payload_not_available": "connection lost",
  "device": {
    "identifiers": [
      "watermeterTest"
    ],
    "name": "watermeter",
    "model": "Meter Digitizer",
    "manufacturer": "AI on the Edge Device",
    "sw_version": "N/A",
    "configuration_url": "http://192.168.1.x"
  }
}
```


#### Binary Sensor (equal to a summary error of the device)

- Format: `homeassistant/binary_sensor/[hostname]/problem`
- Example: `homeassistant/binary_sensor/watermeter/problem`

Discovery Topic Configuration| Description
:-|:-
`homeassistant/sensor/[hostname]/problem/config` | Uptime

Example:
```
{
  "~": "watermeterTest",
  "unique_id": "watermeter-problem",
  "object_id": "watermeter_problem",
  "name": "Problem",
  "icon": "mdi:alert-outline",
  "state_topic": "~/main/value_status",
  "value_template": "{{ 'OFF' if '000 Valid' in value else 'ON'}}",
  "device_class": "problem",
  "availability_topic": "~/connection",
  "payload_available": "connected",
  "payload_not_available": "connection lost",
  "device": {
    "identifiers": [
      "watermeterTest"
    ],
    "name": "watermeter",
    "model": "Meter Digitizer",
    "manufacturer": "AI on the Edge Device",
    "sw_version": "N/A",
    "configuration_url": "http://192.168.1.x"
  }
}
```

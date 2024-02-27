## Overview: MQTT API

The device is capable to register to a MQTT broker to publish data and subscribe to specific topics.

!!! __Note__: 
    Only MQTT v3.1.1 is supported.

### Precondition

#### Parametrization
The MQTT service has to be enabled and configured properly via web interface (`Settings` -> `Configuration` -> section `MQTT`)

The following parameters shall to be configured:
- `URI` (mandatory)
- `Main Topic` (mandatory)
- `Client ID` (mandatory)
- `User` (optional)
- `Password` (optional)
- `TLS Encryption` (optional)
- `Retain Messages` (optional)

#### Main Topic

All data gets located under the `Main Topic` which is defined in device configuration. The main topic can be flat `watermeter` or even be nested multiple times, e.g. `water/mainwatermeter` or `water/building1/meter1/...`.


#### Home Assistant Discovery

see [Home Assistant Discovery](home-assistant-discovery.md)

### Topics

#### Process Control

The device can be controled by publish data to the following topics.

- Format: `[Main Topic]/[Subscribed Topic]`
- Example: `watermeter/ctrl/flow_start`

Topic | Description | Payload
:-|:-|:-
`[Main Topic]/ctrl/flow_start` | Trigger a cyle start | any character, length > 0
`[Main Topic]/ctrl/set_prevalue` | Set the last valid value (fallback value) to given value or the actual raw value | see Usage Details 1.

Usage Details
1. `[Main Topic]/ctrl/set_prevalue`: Set the last valid value (fallback value) to given value or the actual raw value
    Payload (needs to be provided in JSON notation):
    - Set to given value (value >= 0)
      - `"numbersname":`Provide name of number sequence
      - `"value":` provide the value to be set
      - Example:
        ```
        {
          "numbersname": "main",
          "value": 1234.5678
        }
        ```
        
    - Set to actual raw value (value < 0; Precondition: Valid raw value is mandatory)
      - `"numbersname":` Provide name of number sequence  
      - `"value":` Provide any negative number
      - Example:
        ```
        {
          "numbersname": "main",
          "value": -1
        }
        ```

---

#### Device Status (Static)

The following device status topics gets published during the `Publish To MQTT` state **only in the first cycle**.

!!! __Warning__: 
    If you want to use these topics, it is recommended to switch on `Message Retain` flag to avoid loosing topic state whenever your topic consuming system is potentially getting restarted while AI-on-the-Edge running without interruption.

- Format: `[Main Topic]/[Topic]`
- Example: `watermeter/MAC`

Topic | Description | Output
:-|:-|:-
`[Main Topic]/MAC` | Device MAC Address | `44:21:D8:04:DF:A8`
`[Main Topic]/IP` | Device IPv4 Address | `192.168.1.x`
`[Main Topic]/hostname` | Device Hostname | `watermetter`
`[Main Topic]/interval` | [Processing Interval](../../Configuration/Parameter/AutoTimer/Interval.md) [min] | `2.0`

---

#### Device Status

The following device status topics gets published during the `Publish To MQTT` state in **every cycle**.

- Format: `[Main Topic]/[Topic]`
- Example: `watermeter/connection`

Topic | Description | Output
:-|:-|:-
`[Main Topic]/connection` | MQTT Connection Status<br><br>Possible States:<br>- connected<br>- connection lost | `connected`
`[Main Topic]/uptime` | Device Uptime [s] | `147`
`[Main Topic]/freeMem` | Total Free Memory (Int. + Ext.) [kB] | `3058639`
`[Main Topic]/wifiRSSI` | WLAN Signal Strength [dBm] | `-54`
`[Main Topic]/CPUtemp` | Device CPU Temperature (°C) | `45`
`[Main Topic]/status` | Process State | `Idle - Waiting for Autostart`
`[Main Topic]/process_error` | Process Error State<br><br>Possible States:<br>- `0`: No error<br>- `1`: Three process errors in a row | `0`

---

#### Number Sequence Results

- Format: `[Main Topic]/[Sequence Name]/[Topic]`
- Example: `watermeter/main/actual_value`

!!! __Note__: 
    All configured number sequences get published depending on your device configuration.

The following topics gets published during the `Publish To MQTT` state of every cycle.

Topic | Description | Output
:-|:-|:-
`[Main Topic]/[Sequence Name]/actual_value` | Actual value | `146.540`
`[Main Topic]/[Sequence Name]/fallback_value` | Fallback value<br>(Latest valid result) | `146.540`
`[Main Topic]/[Sequence Name]/raw_value` | Raw value <br>The value before performing any post-processing | `146.539`
`[Main Topic]/[Sequence Name]/value_status` | Value Status<br><br>Possible States:<br>`000 Valid`: Valid, no deviation <br>`E90 No data to substitute N`: No valid data to substitude N's (only class-11 models) <br>`E91 Rate negative`: Small negative rate, use fallback value as actual value (info) <br>`E92 Rate too high (<)`: Negative rate larger than specified max rate (error) <br>`E93 Rate too high (>)`: Positive rate larger than specified max rate (error) | `E91 Rate negative`
`[Main Topic]/[Sequence Name]/rate_per_min` | Rate per minute<br>(Delta of the last two valid processed cycles and normalized to minute) | `0.000`
`[Main Topic]/[Sequence Name]/rate_per_processing` | Rate per processing<br>(Delta of the last two valid processed cycles) | `0.000`
`[Main Topic]/[Sequence Name]/rate_per_time_unit` | Rate per HA time unit<br>(Delta of the last two valid processed cycles and normalized to time unit, e.g. minute. The time unit gets derived from Home Assistant [meter type](../../Configuration/Parameter/MQTT/MeterType.md)) | `0.000`
`[Main Topic]/[Sequence Name]/timestamp_processed` | Timestamp of last processed cycle | `2024-02-02T16:59:24+0100`
`[Main Topic]/[Sequence Name]/json` | Provide the following content in JSON notation: `actual_value`, `fallback_vaue`, `raw_value`, `value_status`, `rate_per_min`, `rate_per_processing`, `timestamp_processed` | `{"actual_value": "146.540", "fallback_value": "146.540", "raw_value": "146.539", "value_status": "E91 Rate negative", "rate_per_min": "0.0000", "rate_per_processing": "0.000", "timestamp_processed": "2024-02-02T20:13:54+0100"}`

---

#### GPIO (General Purpose Input / Output Pins)

##### Precondition

The respective GPIO pin need to be enabled in configuration first (section `GPIO`).

##### GPIO Control

GPIO pins can be controlled by publishing data to the following topic.

- Format: `[Main Topic]/GPIO/[GPIO Name]`
- Example: `watermeter/GPIO/GPIO12`

Topic | Description | Payload
:-|:-|:-
`[Main Topic]/GPIO/[GPIO Name]` | Control GPIO Pin State | `HIGH`: `true` or `1`<br>`LOW`: `false` or `0`

---

##### GPIO Status

GPIO pin status gets published to the following topic.

- Format: `[Main Topic]/GPIO/[GPIO Name]`
- Example: `watermeter/GPIO/GPIO12`

Topic | Description | Output
:-|:-|:-
`[Main Topic]/GPIO/[GPIO Name]` | GPIO Pin State<br><br>Possible States:<br>- `HIGH`: `true`<br>- `LOW`: `false` | `false`

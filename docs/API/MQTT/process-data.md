[Overview](_OVERVIEW.md) 

## MQTT API: Process Data

The following topics get published during the `Publish To MQTT` state **every cycle**.

!!! __Note__: 
    All configured number sequences get published depending on your device configuration. 
    Depending on parameter `Process Data Notation` in MQTT section topics get published in 
    JSON notation (`JSON Topic`), as single topics (`Single Topics`) or both notations together
    (`JSON + Topics`).


### Topics

#### 1. Number of seqeunces

  - Format: `[MainTopic]/process/data/[Topic]`
  - Example: `watermeter/process/data/number_sequences`

| Topic                     | Description                 | Output
|:--------------------------|:----------------------------|:--------------     
| `number_sequences`        | Number of configured number sequences| `1`

--- 

#### 2. Publish process data as one JSON topic

Parameter `Process Data Notation`: `JSON` or `JSON + Topics`

  - Format: `[MainTopic]/device/data/[SequenceNumber]/json`
  - Example: `watermeter/device/data/1/json`

| JSON Property             | Description                 | Output
|:--------------------------|:----------------------------|:--------------     
| `json`.`actual_value`     | Actual value                | `146.540`
| `json`.`fallback_value`   | Fallback value              | `146.540`
| `json`.`raw_value`        | Raw value                   | `146.539`
| `json`.`value_status`     | Value Status<br><br>Possible States:<br>`000 Valid`: Valid, no deviation <br>`E90 No data to substitute N`: No valid data to substitude N's (only class-11 models) <br>`E91 Rate negative`: Small negative rate, use fallback value as actual value (info) <br>`E92 Rate too high (<)`: Negative rate larger than specified max rate (error) <br>`E93 Rate too high (>)`: Positive rate larger than specified max rate (error) | `E91 Rate negative`
| `json`.`rate_per_minute`  | Rate per minute<br>(Delta of actual and last valid processed cycle + normalized to minute) | `0.000`
| `json`.`rate_per_interval` | Rate per interval<br>(Delta of actual and last valid processed cycle) | `0.000`
| `json`.`rate_per_time_unit` | Rate per HA time unit<br>(Delta of the last two valid processed cycles and normalized to time unit, e.g. minute. The time unit gets derived from Home Assistant [meter type](../../Configuration/Parameter/MQTT/MeterType.md)) | `0.000`
| `json`.`timestamp_processed` | Timestamp of last processed cycle | `2024-02-02T16:59:24+0100`

---

#### 3. Publish process data as single topics

`Process Data Notation`: `Single Topics` or `JSON + Topics`
  
  - Format: `[MainTopic]/process/data/[SequenceNumber]/[Topic]`
  - Example: `watermeter/process/data/1/actual_value`

| Topic                     | Description                 | Output
|:--------------------------|:----------------------------|:--------------   
| `actual_value`            | Actual value                | `146.540`
| `fallback_value`          | Fallback value              | `146.540`
| `raw_value`               | Raw value                   | `146.539`
| `value_status`            | Value Status<br><br>Possible States:<br>`000 Valid`: Valid, no deviation <br>`E90 No data to substitute N`: No valid data to substitude N's (only class-11 models) <br>`E91 Rate negative`: Small negative rate, use fallback value as actual value (info) <br>`E92 Rate too high (<)`: Negative rate larger than specified max rate (error) <br>`E93 Rate too high (>)`: Positive rate larger than specified max rate (error) | `E91 Rate negative`
| `rate_per_minute`         | Rate per minute<br>(Delta of actual and last valid processed cycle + normalized to minute) | `0.000`
| `rate_per_interval`       | Rate per interval<br>(Delta of actual and last valid processed cycle) | `0.000`
| `rate_per_time_unit`      | Rate per HA time unit<br>(Delta of the last two valid processed cycles and normalized to time unit, e.g. minute. The time unit gets derived from Home Assistant [meter type](../../Configuration/Parameter/MQTT/HAMeterType.md)) | `0.000`
| `timestamp_processed`     | Timestamp of last processed cycle | `2024-02-02T16:59:24+0100`

[Overview](_OVERVIEW.md) 

## REST API endpoint: process_data

`http://IP-ADDRESS/process_data`


Get process related actual data, results and system infos (all content which is visualized on WebUI Overview page)

- JSON: `/process_data`
- HTML: `/process_data?type=xxx` -> Number seqeunce related data provided inline divided by tab
- HTML: `/process_data?type=xxx&sequencename=xxx` -> Number seqeunce related data provided separate per number sequence

The following data are available:

| JSON Property / HTML query `type=`   | Description                                        | Output
|:-------------------------------------|:---------------------------------------------------|:-----------------------
| `api_name`                           | API Name + Version                                 | `process_data:v2`
| `number_sequences`                   | Number Sequence Quantity                           | `1`
| `timestamp_processed`                | Sequence name + Timestamp of last processed cycle per seqeunce<br><br>Notes:<br>- Output of multiple sequences possible<br>- Time of image taken | `main`: `2024-02-02T11:01:29+0100`
| `timestamp_fallbackvalue`            | Sequence name + Timestamp of fallback value per seqeunce<br><br>Notes:<br>- Output of multiple sequences possible<br>- Time of image taken | `main`: `2024-02-02T11:01:29+0100`
| `actual_value`                       | Actual value per seqeunce<br><br>Notes:<br>- Output of multiple sequences possible | `main`:`146.540`
| `fallback_value`                     | Fallback value<br>(Latest valid result) per seqeunce<br><br>Notes:<br>- Output of multiple sequences possible <br>- Possible special states:<br>`Deactivated`: No fallback value usage <br>`Outdated`: Fallback value too old <br>`Not Determinable`: Age of value not determinable | `main`:`146.540`
| `raw_value`                          | Raw value <br>(Value before any post-processing) per seqeunce<br><br>Notes:<br>- Output of multiple sequences possible | `main`:`146.539`
| `value_status`                       | Value Status per sequence<br><br>Notes:<br>- Output of multiple sequences possible <br>- Possible states:<br>`000 Valid`: Valid, no deviation <br>`W01 W01 Empty data`: No data available <br>`E90 No data to substitute N`: No valid data to substitude N's (only class-11 models) <br>`E91 Rate negative`: Small negative rate, use fallback value as actual value (info) <br>`E92 Rate too high (<)`: Negative rate larger than specified max rate (error) <br>`E93 Rate too high (>)`: Positive rate larger than specified max rate (error) | `main`:`000 Valid`
| `rate_per_minute`                    | Rate per minute per sequence<br>(Delta between actual and last valid processed value (Fallback Value) + additionally normalized to a minute)<br><br>Notes:<br>- Tab separated listing<br>- Output of multiple sequences possible | `main`:`0.0000`
| `rate_per_interval`                  | Rate per interval per serquence<br>(Delta between actual and last valid processed value (Fallback Value))<br><br>Notes:<br>- Output of multiple sequences possible | `main`:`0.0000`
| `process_status`                     | Process Status<br><br>Possible States:<br>- `Processing (Automatic)`:  Timer-controlled automatic processing<br>- `Processing (Triggered Only)`: Manual triggered processing only<br>- `Not Processing / Not Ready`: Initializing / Initialization failed | `Processing (Automatic)`
| `process_interval`                   | Process Interval [min]                              | `2.0`
| `process_time`                       | Process Time [sec]                                  | `25`
| `process_state`                      | Process State                                       | `[11:01:42] Idle - Waiting for Autostart`
| `process_error`                      | Process Error State<br>- Error definition: Process error with cycle abortion, e.g. alignment failed<br>- Deviation definition: Process deviation with cycle continuation, e.g. rate limit exceeded<br><br>Possible States:<br>- `0`: No error/deviation<br>- `-1`: One error occured<br>- `-2`: Multiple process errors in a row<br>- `1`: One process deviation occured<br>- `2`: Multiple process deviations in a row | `0`
| `device_uptime`                      | Device Uptime [sec]                                 | `496`
| `cycle_counter`                      | Number of processed cycles                          | `64`
| `wlan_rssi`                          | WLAN Signal Strength [dBm]                          | `-58`


1. JSON:
    - Payload:
      - No payload needed
    - Response:
      - Content type: `JSON`
      - Content: JSON response
    - Example: 
```
{
    "api_name": "process_data:v3",
    "number_sequences": 1,
    "timestamp_processed": {
        "main": "2024-09-11T19:07:11+0200"
    },
    "timestamp_fallbackvalue": {
        "main": "2024-09-11T19:07:11+0200"
    },
    "actual_value": {
        "main": "530.06373"
    },
    "fallback_value": {
        "main": "530.06373"
    },
    "raw_value": {
        "main": "00530.06373"
    },
    "value_status": {
        "main": "000 Valid"
    },
    "rate_per_minute": {
        "main": "0.004020"
    },
    "rate_per_interval": {
        "main": "0.00201"
    },
    "process_status": "Processing (Automatic)",
    "process_interval": 0.5,
    "process_time": 22,
    "process_state": "[19:07:37] Idle - Waiting For Autostart",
    "process_error": 0,
    "device_uptime": 1072,
    "cycle_counter": 34,
    "wlan_rssi": -55
}
```

2. HTML query request:
    - Payload:
      - `/process_data?type=___`
    - Response:
      - Content type: `HTML`
      - Content: HTML query response
    - Example: `/process_data?type=process_status` 

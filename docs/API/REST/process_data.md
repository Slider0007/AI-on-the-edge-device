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
| `timestamp_processed`.`inline`       | Sequence name + Timestamp of last processed cycle<br><br>Notes:<br>- Tab separated listing<br>- Output of multiple sequences possible<br>- Time of image taken | `main\t2024-02-02T11:01:29+0100`
| `timestamp_processed`.`sequence`     | Sequence name + Timestamp of last processed cycle per seqeunce<br><br>Notes:<br>- Output of multiple sequences possible<br>- Time of image taken | `main`: `2024-02-02T11:01:29+0100`
| `timestamp_fallbackvalue`.`inline`   | Sequence name + Timestamp of fallback value<br><br>Notes:<br>- Tab separated listing<br>- Output of multiple sequences possible<br>- Time of image taken | `main\t2024-02-02T11:01:29+0100`
| `timestamp_fallbackvalue`.`sequence` | Sequence name + Timestamp of fallback value per seqeunce<br><br>Notes:<br>- Output of multiple sequences possible<br>- Time of image taken | `main`: `2024-02-02T11:01:29+0100`
| `actual_value`.`inline`              | Actual value<br><br>Notes:<br>- Tab separated listing<br>- Output of multiple sequences possible | `main\t146.540`
| `actual_value`.`sequence`            | Actual value per seqeunce<br><br>Notes:<br>- Output of multiple sequences possible | `main`:`146.540`
| `fallback_value`.`inline`            | Fallback value<br>(Latest valid result)<br><br>Notes:<br>- Tab separated listing<br>- Output of multiple sequences possible | `main\t146.540`
| `fallback_value`.`sequence`          | Fallback value<br>(Latest valid result) per seqeunce<br><br>Notes:<br>- Output of multiple sequences possible | `main`:`146.540`
| `raw_value`.`inline`                 | Raw value <br>(Value before any post-processing)<br><br>Notes:<br>- Tab separated listing<br>- Output of multiple sequences possible | `main\t146.539`
| `raw_value`.`sequence`               | Raw value <br>(Value before any post-processing) per seqeunce<br><br>Notes:<br>- Output of multiple sequences possible | `main`:`146.539`
| `value_status`.`inline`              | Value Status<br><br>Notes:<br>- Tab separated listing<br>- Output of multiple sequences possible <br>- Possible states:<br>`000 Valid`: Valid, no deviation <br>`E90 No data to substitute N`: No valid data to substitude N's (only class-11 models) <br>`E91 Rate negative`: Small negative rate, use fallback value as actual value (info) <br>`E92 Rate too high (<)`: Negative rate larger than specified max rate (error) <br>`E93 Rate too high (>)`: Positive rate larger than specified max rate (error)| `main\tE91 Rate negative`
| `value_status`.`sequence`            | Value Status per sequence<br><br>Notes:<br>- Output of multiple sequences possible <br>- Possible states:<br>`000 Valid`: Valid, no deviation <br>`E90 No data to substitute N`: No valid data to substitude N's (only class-11 models) <br>`E91 Rate negative`: Small negative rate, use fallback value as actual value (info) <br>`E92 Rate too high (<)`: Negative rate larger than specified max rate (error) <br>`E93 Rate too high (>)`: Positive rate larger than specified max rate (error)| `main`:`E91 Rate negative`
| `rate_per_minute`.`inline`           | Rate per minute<br>(Delta of the last two valid processed cycles and normalized to minute)<br><br>Notes:<br>- Tab separated listing<br>- Output of multiple sequences possible | `main\t0.0000`
| `rate_per_minute`.`sequence`         | Rate per minute per sequence<br>(Delta of the last two valid processed cycles and normalized to minute)<br><br>Notes:<br>- Tab separated listing<br>- Output of multiple sequences possible | `main`:`0.0000`
| `rate_per_processing`.`inline`       | Rate per processing<br>(Delta of the last two valid processed cycles)<br><br>Notes:<br>- Tab separated listing<br>- Output of multiple sequences possible | `main\t0.0000`
| `rate_per_processing`.`sequence`     | Rate per processing per serquence<br>(Delta of the last two valid processed cycles)<br><br>Notes:<br>- Output of multiple sequences possible | `main`:`0.0000`
| `process_status`                     | Process Status<br><br>Possible states:<br>- `Processing (Automatic)`<br>- `Processing (Triggered Only)`<br>- `Not Processing / Not Ready` | `Processing (Automatic)`
| `process_interval`                   | Processing Interval [min]                           | `2.0`
| `process_time`                       | Processing Time [sec]                               | `25`
| `process_state`                      | Process State                                       | `[11:01:42] Idle - Waiting for Autostart`
| `process_error`                      | Process Error State<br><br>Possible states:<br>- `0`: No error<br>- `-1`: Single process error<br>- `-2`: Three process errors in a row | `0`
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
    "api_name": "process_data:v2",
    "number_sequences": 1,
    "timestamp_processed": {
        "inline": "main\t2024-02-24T13:59:04+0100",
        "sequence": {
            "main": "2024-02-24T13:59:04+0100"
        }
    },
    "timestamp_fallbackvalue": {
        "inline": "main\t2024-02-24T13:59:04+0100",
        "sequence": {
            "main": "2024-02-24T13:59:04+0100"
        }
    },
    "actual_value": {
        "inline": "main\t530.00920",
        "sequence": {
            "main": "530.00920"
        }
    },
    "fallback_value": {
        "inline": "main\t530.00920",
        "sequence": {
            "main": "530.00920"
        }
    },
    "raw_value": {
        "inline": "main\t00530.00920",
        "sequence": {
            "main": "00530.00920"
        }
    },
    "value_status": {
        "inline": "main\t000 Valid",
        "sequence": {
            "main": "000 Valid"
        }
    },
    "rate_per_minute": {
        "inline": "main\t0.001830",
        "sequence": {
            "main": "0.001830"
        }
    },
    "rate_per_processing": {
        "inline": "main\t0.00061",
        "sequence": {
            "main": "0.00061"
        }
    },
    "process_status": "Processing (Automatic)",
    "process_interval": 2.0,
    "process_time": 15,
    "process_state": "[13:59:17] Idle - Waiting for Autostart",
    "process_error": 0,
    "device_uptime": 496,
    "cycle_counter": 4,
    "wlan_rssi": -60
}
```

2. HTML query request:
    - Payload:
      - `/process_data?type=___`
    - Response:
      - Content type: `HTML`
      - Content: HTML query response
    - Example: `/process_data?type=process_status` 

[Overview](_OVERVIEW.md) 

### REST API endpoint: process_data

`http://IP-ADDRESS/process_data`


Get process related actual data, results and system infos (all content which is visualized on WebUI Overview page)

The following data are available:

| JSON Property                        | Description                                        | Output
|:-------------------------------------|:---------------------------------------------------|:-----------------------
| `api_name`                           | Name of the API                                    | `process_data`
| `timestamp_processed`                | Sequence name + Timestamp of last processed cycle<br><br>Notes:<br>- Tab separated listing<br>- Output of multiple sequences possible<br>- Time of image taken | `main\t2024-02-02T11:01:29+0100`
| `timestamp_fallbackvalue`            | Sequence name + Timestamp of fallback value<br><br>Notes:<br>- Tab separated listing<br>- Output of multiple sequences possible<br>- Time of image taken | `main\t2024-02-02T11:01:29+0100`
| `actual_value`                       | Actual value<br><br>Notes:<br>- Tab separated listing<br>- Output of multiple sequences possible | `main\t146.540`
| `fallback_value`                     | Fallback value<br>(Latest valid result)<br><br>Notes:<br>- Tab separated listing<br>- Output of multiple sequences possible | `main\t146.540`
| `raw_value`                          | Raw value <br>(Value before any post-processing)<br><br>Notes:<br>- Tab separated listing<br>- Output of multiple sequences possible | `main\t146.539`
| `value_status`                       | Value Status<br><br>Notes:<br>- Tab separated listing<br>- Output of multiple sequences possible <br>- Possible states:<br>`000 Valid`: Valid, no deviation <br>`E90 No data to substitute N`: No valid data to substitude N's (only class-11 models) <br>`E91 Rate negative`: Small negative rate, use fallback value as actual value (info) <br>`E92 Rate too high (<)`: Negative rate larger than specified max rate (error) <br>`E93 Rate too high (>)`: Positive rate larger than specified max rate (error)| `main\tE91 Rate negative`
| `rate_per_min`                       | Rate per minute<br>(Delta of the last two valid processed cycles and normalized to minute)<br><br>Notes:<br>- Tab separated listing<br>- Output of multiple sequences possible | `main\t0.0000`
| `rate_per_processing`                | Rate per processing<br>(Delta of the last two valid processed cycles)<br><br>Notes:<br>- Tab separated listing<br>- Output of multiple sequences possible | `main\t0.0000`
| `process_state`                      | Process State | `[11:01:42] Idle - Waiting for Autostart`
| `process_error`                      | Process Error State<br><br>Possible States:<br>- `0`: No error<br>- `1`: Three process errors in a row | `0`
| `temperature`                        | Device Temperature (degree celcius)                 | `38`
| `rssi`                               | WLAN Signal Strength (dBm)                          | `-54`
| `uptime`                             | Device Uptime (Formated)                            | `  5d 16h 38m 06s`
| `cycle_counter`                      | Number of processed cycles                          | `4100`


Payload:
- No payload needed

Response:
- Content type: `JSON`
- Content: Query response
- Example: 
```
{
  "api_name": "process_data",
  "timestamp_processed": "main\t2024-02-02T11:01:29+0100",
  "timestamp_fallbackvalue": "main\t2024-02-02T11:01:29+0100",
  "actual_value": "main\t146.540",
  "fallback_value": "main\t146.540",
  "raw_value": "main\t146.539",
  "value_status": "main\tE91 Rate negative",
  "rate_per_min": "main\t0.0000",
  "rate_per_processing": "main\t0.000",
  "process_state": "[11:01:42] Idle - Waiting for Autostart",
  "process_error": "0",
  "temperature": "38",
  "rssi": "-54",
  "uptime": "  5d 16h 38m 06s",
  "cycle_counter": "4100"
}
```

[Overview](_OVERVIEW.md) 

### REST API endpoint: json

`http://IP-ADDRESS/json`


Get process related actual data and results per number sequence

The following data are available:

| JSON Property                        | Description                                        | Output
|:-------------------------------------|:---------------------------------------------------|:-----------------------
| `actual_value`                       | Actual value | `146.540`
| `fallback_value`                     | Fallback value<br>(Latest valid result) | `146.540`
| `raw_value`                          | Raw value <br>(Raw value without any post-prcoessing) | `146.539`
| `value_status`                       | Value Status <br><br>Possible States:<br>`000 Valid`: Valid, no deviation <br>`E90 No data to substitute N`: No valid data to substitude N's (only class-11 models) <br>`E91 Rate negative`: Small negative rate, use fallback value as actual value (info) <br>`E92 Rate too high (<)`: Negative rate larger than specified max rate (error) <br>`E93 Rate too high (>)`: Positive rate larger than specified max rate (error)| `E91 Rate negative`
| `rate_per_min`                       | Rate per minute<br>(Delta of the last two valid processed cycles and normalized to minute) | `0.0000`
| `rate_per_prcoessing`                | Rate per processing<br>(Delta of the last two valid processed cycles) | `0.0000`
| `timestamp_processed`                | Timestamp of last processed cycle (Time of image taken) | `2024-02-02T11:01:29+0100`


Payload:
- No payload needed

Response:
- Content type: `JSON`
- Content: Query response
- Example: 
```
{
  "main": {
    "actual_value": "146.540",
    "fallback_value": "146.540",
    "raw_value": "146.539",
    "value_status": "E91 Rate negative",
    "rate_per_min": "0.0000",
    "rate_per_processing": "0.000",
    "timestamp_processed": "2024-02-02T14:28:01+0100"
  }
}
```

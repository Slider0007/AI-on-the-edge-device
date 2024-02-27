[Overview](_OVERVIEW.md) 

### REST API endpoint: datafileact

`http://IP-ADDRESS/datafileact`


Get data entries from today (only last 80kB)

Each row represents one processed cycle.

CSV separated format: `time, sequence name, raw value, actual value, fallback value, rate per min, rate per processing, value status, results of digit ROIs, results of analog ROIs`

Process status:
- `000`: Valid, no deviation
- `E91`: Small negative rate, use fallback value as actual value (information only)
- `E92`: Negative rate larger than specified max rate (error)
- `E93`: Positive rate larger than specified max rate (error)


Payload:
- No payload needed

Response:
- Content type: `HTML`
- Content: Content of file
- Example: `2024-02-01T14:39:33+0100,main,146.364,146.364,146.364,0.0000,0.000,000,1.2,4.0,6.3,3.5,6.4`

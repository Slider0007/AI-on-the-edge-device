[Overview](_OVERVIEW.md) 

## REST API endpoint: log

`http://IP-ADDRESS/log`


Get logs from log file 


### General

Each row represents one log entry.

Format: `[uptime] timestamp <log level> [source] message`

Log levels:
- `<ERR>`: Error
- `<WRN>`: Warning
- `<INF>`: Info
- `<DBG>`: Debug


### Syntax

1. Get last entries (max. 80kB) of today's log file
    - Payload:
      - `/log` No payload parameter needed
    - Response:
      - Content type: `HTML`
      - Content: Content of file
      - Example: `[0d02h49m24s] 2024-02-01T15:54:07	<DBG>	[FLOWCTRL] Status: Aligning (15:54:07)`

2. Get all entries of today's log file
    - Payload:
      - `/log?type=full`
    - Response:
      - Content type: `HTML`
      - Content: Content of file
      - Example: `[0d02h49m24s] 2024-02-01T15:54:07	<DBG>	[FLOWCTRL] Status: Aligning (15:54:07)`

3. Get API name and version:
    - Payload:
      - `/log?type=api_name`
    - Response:
      - Content type: `HTML`
      - Content: HTML query response, e.g. `log:vX`


!!! __Tip__: 
    Get log entries from previous days: Use [/fileserver](fileserver.md) endpoint.

[Overview](_OVERVIEW.md) 

### REST API endpoint: logfileact

`http://IP-ADDRESS/logfileact`


Get log entries from today (only last 80kB)

Each row represents one log entry.

Format: `[uptime] timestamp <log level> [source] message`

Log levels:
- `<ERR>`: Error
- `<WRN>`: Warning
- `<INF>`: Info
- `<DBG>`: Debug


Payload:
- No payload needed

Response:
- Content type: `HTML`
- Content: Content of file
- Example: `[0d02h49m24s] 2024-02-01T15:54:07	<DBG>	[FLOWCTRL] Status: Aligning (15:54:07)`

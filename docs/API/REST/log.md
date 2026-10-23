[Overview](_OVERVIEW.md) 

### REST API Endpoint: `/log`

`http://IP-ADDRESS/log`

Retrieve and query system log files with support for metadata, full dumps, and time-range filtering.

---

### General Information

* **Content-Type**: `text/plain`
* **Cache Control**: `no-cache`
* **Content Format**: Each row represents one message (TXT format, tab-separated):
`[uptime] timestamp <log level> [source] message`
* **Log Levels**:
  * `<ERR>`: Error
  * `<WRN>`: Warning
  * `<INF>`: Info
  * `<DBG>`: Debug

---

### Query Parameters & Syntax

#### 1. Get Recent Log Entries (Default)
* **Payload**: `/log` (No query parameters required)
* **Description**: Returns the tail (last portion) of the most recent log file.
* **Example Response**: 
  `[0d02h49m24s] 2024-02-01T15:54:07	<DBG>	[FLOWCTRL] Status: ...`

#### 2. Get Logs by Timestamp Range
* **Payload**: `/log?start=YYYY-MM-DDTHH:MM:SS&end=YYYY-MM-DDTHH:MM:SS`
* **Description**: Filters and streams log entries across multiple log files matching the given ISO timestamp range (inclusive, ordered oldest to newest). 
* **Note**: If the output exceeds the maximum stream size limit (2 MB), the response is safely truncated with a `... --TRUNCATED_SIZELIMIT--\n` marker.

#### 3. Get Full Data Entries (unlimited)
* **Payload**: `/log?type=full` (Optional: can be combined with `start` and/or `end` parameters)
* **Description**: Streams all log entries across available files without size limits. When filtered with a date range, returns all matching entries chronologically.

#### 4. Get Log File List (JSON-coded)
* **Payload**: `/log?type=filelist`
* **Description**: Returns a list of available log files stored in the root logs directory (JSON).

#### 5. Get API Name and Version
* **Payload**: `/log?type=api_name`
* **Description**: Returns the current API identifier and version string (`log:v3`).

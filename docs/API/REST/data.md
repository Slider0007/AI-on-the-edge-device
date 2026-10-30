[Overview](_OVERVIEW.md) 

## REST API Endpoint: `data`

`http://IP-ADDRESS/data`

Retrieve and query system data and CSV log files with support for metadata, full dumps, and file listings.

---

### General Information

* **Content-Type**: `text/plain` (query for filelist: `application/json`)`
* **Cache Control**: `no-cache`
* **Content Format**: Each row represents one processed cycle (CSV format, comma-separated):
  `time, sequence name, raw value, actual value, fallback value, rate per min, absolute change, process status, results of digit ROIs, results of analog ROIs`
* **Process Status Codes**:
  * `000`: Valid, no deviation
  * `E91`: Small negative rate, use fallback value as actual value (information only)
  * `E92`: Negative rate larger than specified max rate (error)
  * `E93`: Positive rate larger than specified max rate (error)

---

### Query Parameters & Syntax

#### 1. Get Recent Data Entries (Default)
* **Payload**: `/data` (No query parameters required)
* **Description**: Returns the last entries (tail portion) of today's data file.
* **Example Response**: 
  `2024-02-01T14:39:33+0100,main,146.364,146.364,146.364,0.0000,0.000,000,1.2,4.0,6.3,3.5,6.4`

#### 2. Get Full Data Entries
* **Payload**: `/data?type=full`
* **Description**: Returns all entries of today's data file without size limits.
* **Example Response**: 
  `2024-02-01T14:39:33+0100,main,146.364,146.364,146.364,0.0000,0.000,000,1.2,4.0,6.3,3.5,6.4`

#### 3. Get Data File List (JSON-coded)
* **Payload**: `/data?type=filelist`
* **Description**: Returns a list of available CSV data files stored in the root data directory (JSON).

#### 4. Get API Name and Version
* **Payload**: `/data?type=api_name`
* **Description**: Returns the current API identifier and version string (e.g. `data:vX`).

[Overview](_OVERVIEW.md) 

## REST API Endpoint: `fileserver`

`http://IP-ADDRESS/fileserver/<path>`

Browse, list and download files and directories from the SD card via HTTP GET requests.

---

### General Information

* **HTTP Method**: `GET`
* **Cache Control**: `no-cache`

---

### Request Syntax & Operations

#### 1. Directory Listing & File Management
* **Payload / URL**: `/fileserver/<dir-path>/` (Must end with a trailing slash `/`)
* **Query Parameters**:
  * `?filelist=true`: Returns a JSON-formatted list of files in the directory.
  * `?readonly=true` (or `false`): Sets the directory view mode.
  * `?full=true` (or `false`): Controls whether to show all files (`showAllFiles`).
* **Response**: HTML formatted directory view or JSON file list.

#### 2. File Download
* **Payload / URL**: `/fileserver/<file-path>` (No trailing slash)
* **Description**: Downloads the specified file from storage.
* **Response**: 
  * **Content-Type**: Automatically determined based on file extension (`setContentTypeFromFile`).
  * **Content**: Binary file stream chunked via scratch buffer.

---

### Limitations & Error Responses

* **Path Malformed or Too Long (`400 Bad Request`)**: Fails if the path exceeds `FILE_PATH_MAX` (255).
* **File Not Found (`404 Not Found`)**: Fails if the requested file or directory does not exist on the filesystem.
* **Server Read/Stream Failures (`500 Internal Server Error` / `ESP_FAIL`)**: Fails if the file cannot be read or chunks fail to send over the connection.
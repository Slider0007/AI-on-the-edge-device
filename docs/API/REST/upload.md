[Overview](_OVERVIEW.md) 

## REST API Endpoint: `upload`

`http://IP-ADDRESS/upload/<path>`

Upload files to the SD card via an HTTP POST request.

---

### General Information

* **HTTP Method**: `POST`
* **Response Headers**: 
  * Redirects (`303 See Other`) include a `Location` header pointing to the parent directory (with `?full=false`).
  * Protected files return HTTP `200 OK` with a plain text success message to prevent folder reloading.

---

### Request Syntax & Operations

#### 1. Upload a File
* **Payload / URL**: `/upload/<file-path>` (e.g., `/upload/config/settings.json`)
* **Request Body**: Raw file content (subject to maximum size constraints).
* **Query Parameters**:
  * `?overwrite=true`: Overwrites an existing file
* **Response Behavior**:
  * **Standard Files**: Returns an HTTP `303 See Other` status code with a `Location` header redirecting to the parent directory.
  * **Protected / Non-Redirect Files**: Returns HTTP `200 OK` with a plain text message (`File saved successfully: <filename>`).

---

### Limitations & Error Responses

* **Path Format (`400 Bad Request`)**: 
  * Fails if it is improperly included when attempting to delete a single file
* **Path Length (`414 URI Too Long`)**: 
  * Fails if the target path exceeds the maximum allowed length (`FILE_PATH_MAX` == 255) or is malformed
* **File Size Limit Exceeded (`413 Content too large`)**: 
  * Fails if `content_len` is greater than `MAX_FILE_SIZE` (16MB)
* **File Already Exists (`400 Bad Request`)**: 
  * Fails if a file already exists at the specified destination path. (only applicable without overwrite parameter)
* **Write Failures (`500 Internal Server Error`)**: 
  * Fails if the request body stream cannot be fully written to storage

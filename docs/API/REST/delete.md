[Overview](_OVERVIEW.md) 

## REST API Endpoint: `delete`

`http://IP-ADDRESS/delete/<path>`

Delete individual files or recursively remove entire directories from the SD card via an HTTP POST request.

---

### General Information

* **HTTP Method**: `POST`
* **Response Headers**: 
  * Redirects (`303 See Other`) include a `Location` header pointing to the parent directory (with `?full=false`).
  * `Access-Control-Expose-Headers: Location` is set for file deletions that trigger a redirect.

---

### Request Syntax & Operations

#### 1. Delete a Single File
* **Payload / URL**: `/delete/<file-path>` (e.g., `/delete/log/message/log_2024-01-11.txt`)
* **Description**: Deletes the specified file from the storage.
* **Response Behavior**:
  * **Default / Redirected Files**: Returns an HTTP `303 See Other` status code with a `Location` header redirecting to the parent directory.
  * **Protected / Non-Redirect Files (`noRedirectFiles`)**: Returns HTTP `200 OK` with a plain text message (`File deleted successfully: <filename>`).

#### 2. Delete a Directory Recursively
* **Payload / URL**: `/delete/<dir-path>/?task=deldir` (e.g., `/delete/config/?task=deldir`)
* **Description**: Recursively deletes all files and contents within the specified directory (**Warning**: Irreversible operation).
* **Response**: Returns HTTP `303 See Other` redirecting to the parent directory (`<parent-path>?full=false`).

---

### Limitations & Error Responses

* **Path Length & Format (`400 Bad Request`)**: 
  * Fails if the path exceeds the maximum allowed length (`FILE_PATH_MAX` == 255) or is malformed.
  * Fails if a trailing slash is improperly included when attempting to delete a single file.
* **File Not Found (`400 Bad Request`)**: 
  * Fails if the target file does not exist on the filesystem (`stat` check fails).
* **Server Execution Failures (`500 Internal Server Error`)**: 
  * Fails if the underlying system call to delete the file (`unlink`) returns an error.
  * Fails if recursive directory contents cannot be fully removed (`deleteAllFilesInDirectory` fails).

---

> **Tip**: Ensure proper path formatting and check file permissions or existence before triggering deletions.
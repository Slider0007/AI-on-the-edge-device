[Overview](_OVERVIEW.md) 

## REST API Endpoint: `editflow`

`http://IP-ADDRESS/editflow?task=<action>`

Helper API to interact with, set up, and configure the process flow (e.g., copying configuration/marker files or cropping alignment reference regions).

---

### General Information

* **HTTP Method**: `GET`
* **Content-Type**: `text/plain`
* **Parameter Handling**: Uses query strings for task execution and parameters.

---

### Request Syntax & Tasks (`?task=<action>`)

#### 1. Copy File (`task=copy`)
* **Description**: Copies a file from a source path to a destination path on the SD card.
* **Mandatory Parameters**:
  * `in`: Source file path relative to storage root (e.g., `/config/marker1.jpg`)
  * `out`: Destination file path relative to storage root (e.g., `/img_tmp/marker1.jpg`)
* **Example Payload**: `/editflow?task=copy&in=/config/marker1.jpg&out=/img_tmp/marker1.jpg`
* **Response**: Plain text (`Copy Done`)

#### 2. Cut Reference Image / Marker (`task=cutref`)
* **Description**: Crops a designated rectangular region out of a source image and saves it as a JPEG file. Handles memory optimization for 4MB SPIRAM architectures.
* **Mandatory Parameters**:
  * `in`: Source file path (e.g., `/config/reference.jpg`)
  * `out`: Destination file path (e.g., `/img_tmp/marker1.jpg`)
  * `x`: Start X-coordinate of the crop area (e.g., `152`)
  * `y`: Start Y-coordinate of the crop area (e.g., `138`)
  * `dx`: Width of the crop area (e.g., `44`)
  * `dy`: Height of the crop area (e.g., `17`)
* **Example Payload**: `/editflow?task=cutref&in=/config/reference.jpg&out=/img_tmp/marker1_org.jpg&x=152&y=138&dx=44&dy=17`
* **Response**: Plain text (`CutImage Done`)

#### 3. Get API Name and Version (`task=api_name`)
* **Description**: Returns the current API identifier and version string.
* **Example Payload**: `/editflow?task=api_name`
* **Response**: `editflow:v4`

---

### Limitations & Error Responses

* **Flow Not Initialized (`403 Forbidden`)**: 
  * Returns `E90: Request rejected, flow not initialized` if a task like `cutref` is attempted before the system flow is initialized.
* **Method Not Allowed / Busy (`405 Method Not Allowed`)**: 
  * Returns `E91: Request rejected, flow in process | Actual State: ...` if a restricted operation (`cutref`) is requested during active image processing on 4MB SPIRAM configurations.
* **Task Not Found (`404 Not Found`)**: 
  * Returns `E92: Task not found` if an invalid or unsupported `task` parameter is provided.
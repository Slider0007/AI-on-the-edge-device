[Overview](_OVERVIEW.md) 

## REST API Endpoint: `recognition_details`

`http://IP-ADDRESS/recognition_details`

Retrieve an interactive HTML page visualizing the details of the last fully completed image recognition cycle.

---

### General Information

* **HTTP Method**: `GET`
* **Content-Type**: `text/html` (or `text/plain` for metadata requests)
* **Features**: Retrieve an interactive HTML page visualizing the details of the last fully completed image recognition cycle.

---

### Request Syntax & Operations

#### 1. Get Recognition Details HTML Page (Default)
* **Payload**: `/recognition_details` (No parameters required)
* **Description**: Renders a comprehensive HTML dashboard showing image recognition results.
  * **Processing / Uninitialized State**: If the system is uninitialized or currently processing, it displays a progress warning box indicating the current process state along with a refresh button.
  * **Completed State**: Iterates through all sequences to display:
    * **Results Table**: Comparison of **Raw Value** vs. **Actual Value**.
    * **Digit & Analog ROIs**: Individual ROI sections (rendered only if enabled in configuration).
    * **Processed Image**: Full image with overlays loaded from `/img_tmp/alg_roi.jpg`.

#### 2. Get API Name and Version
* **Payload**: `/recognition_details?type=api_name`
* **Description**: Returns the current API identifier and version string.
* **Response**: `text/plain` response containing e.g `recognition_details:vX`.

---

> **Tip**: This endpoint is designed to be embedded as an iframe or viewed directly in a browser to inspect the latest AI processing results and region-of-interest extractions.
[Overview](_OVERVIEW.md) 

## REST API Endpoint: `reboot`

`http://IP-ADDRESS/reboot`

Trigger a controlled device reboot.

---

### General Information

* **HTTP Method**: `GET`
* **Content-Type**: `text/plain`
* **Cache Control**: `no-cache`
* **Description**: Sends a confirmation message and immediately initiates a system reboot.

---

### Request Syntax & Operations

* **Payload**: None (No parameters required)
* **Response**: 
  * **Content**: `Reboot initiated`
  * **Behavior**: The device sends the response text back to the client and triggers an instant hardware restart (`doReboot`).
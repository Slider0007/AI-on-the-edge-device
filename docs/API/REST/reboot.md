[Overview](_OVERVIEW.md) 

## REST API Endpoint: `reboot`

`http://IP-ADDRESS/reboot`

Trigger an instant device reboot.

---

### General Information

* **HTTP Method**: `GET` (or `POST`)
* **Content-Type**: `text/plain`
* **Cache Control**: `no-cache`
* **Description**: Sends a confirmation message and immediately initiates a system reboot.

---

### Request Syntax & Operations

* **Payload**: None (No parameters required)
* **Response**: 
  * **Content**: `Reboot initiated`
  * **Behavior**: The device sends the response text back to the client and triggers an instant hardware restart (`doReboot`).
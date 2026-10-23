[Overview](_OVERVIEW.md) 

## REST API Endpoint: `cycle_start`

`http://IP-ADDRESS/cycle_start`

Manually trigger, schedule, or force the start of a new process cycle.

---

### General Information

* **Content-Type**: `text/plain`
* **Description**: Allows manual start of process cycle, enabling immediate cycle re-triggering, scheduling during active tasks or aborting initialization delays.

---

### Request Syntax & Response Messages

#### 1. Trigger Cycle Start
* **Payload**: None (No parameters required)
* **Response Messages**: 
  * **`001` (Immediate Trigger)**: Returned when the system is idle or when a previous initialization failed. Forces an immediate cycle start.
    * *Example*: `001: Cycle start triggered by REST API (15:54:07)`
  * **`002` (Scheduled Trigger)**: Returned when the system is currently busy (e.g., processing images, publishing data, or running additional tasks). The request is registered and scheduled.
    * *Example*: `002: Cycle start triggered by REST API got scheduled (15:54:07)`
  * **`003` (Abort Delay)**: Returned when the system is in a delayed initialization state. Aborts the delay and starts the cycle immediately.
    * *Example*: `003: Cycle start triggered by REST API abort initialization delay (15:54:07)`
  * **`E90` (Rejected)**: Returned when the request is rejected because the main task is not initialized.
    * *Example*: `E90: Cycle start triggered by REST API. Main task not initialized. Request rejected (15:54:07)`
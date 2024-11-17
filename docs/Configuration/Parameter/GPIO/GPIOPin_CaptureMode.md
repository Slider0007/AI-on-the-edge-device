# Parameter: Pin Capture Mode

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Pin Capture Mode    | capturemode
| Default Value     | `Cyclic Polling`    | `cyclic-polling`
| Input Options     | `Cyclic Polling`<br>`Interrupt Rising Edge`<br>`Interrupt Falling Edge`<br>`Interrupt Rising+Falling` | `cyclic-polling`<br>`interrupt-rising-edge`<br>`interrupt-falling-edge`<br>`interrupt-rising-falling`



## Description

Pin capture mode (only for input GPIO pin mode).<br>
This defines how the selected GPIO input is captured internally.


| Input Option               | Description
|:---                        |:---
| `Cyclic Polling`           | Poll GPIO input state in a predefined interval of 1 second
| `Interrupt Rising Edge`    | Capture GPIO input state when a rising edge of signal is detected
| `Interrupt Falling Edge`   | Capture GPIO input state when a falling edge of signal is detected
| `Interrupt Rising+Falling` | Capture GPIO input state when a rising or falling edge of signal is detected


!!! Tip
    To debounce the GPIO input, use any interrupt capture mode.  
    Debounce time can be defined with parameter `Input Debounce Time`.


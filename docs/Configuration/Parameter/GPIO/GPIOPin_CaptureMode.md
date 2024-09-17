# Parameter: Pin Capture Mode

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Pin Capture Mode    | capturemode
| Default Value     | `cyclic polling`    | `cyclic-polling`
| Input Options     | `cyclic polling`<br>`interrupt rising edge`<br>`interrupt falling edge`<br>`interrupt rising falling` | `cyclic-polling`<br>`interrupt-rising-edge`<br>`interrupt-falling-edge`<br>`interrupt-rising-falling`



## Description

Pin capture mode (only for input GPIO pin mode).<br>
This defines how the selected GPIO input is captured internally.


| Input Option               | Description
|:---                        |:---
| `cyclic polling`           | Poll GPIO input state in a predefined interval of 1 second
| `interrupt rising edge`    | Capture GPIO input state when a rising edge of signal is detected
| `interrupt falling edge`   | Capture GPIO input state when a falling edge of signal is detected
| `interrupt rising falling` | Capture GPIO input state when a rising or falling edge of signal is detected


!!! Tip
    To debounce the GPIO input, use any interrupt capture mode.  
    Debounce time can be defined with parameter `Input Debounce Time`.


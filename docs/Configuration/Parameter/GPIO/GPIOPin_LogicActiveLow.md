# Parameter: Pin Logic level

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Pin Logic Level     | logicactivelow
| Default Value     | `Active High`       | `false`
| Input Options     | `Active High`<br>`Active Low` | `false`<br>`true`



## Description

Defines the pin logic level configuration.


| Input Option               | Description
|:---                        |:---
| `Active High`              | Active state when signal level is high (positive logic, activate by pulling pin to supply level (0 -> 1))
| `Active Low`               | Active state when signal level is low (negative logic, activate by pulling pin to ground level (1 -> 0))


!!! Note
    This option is only available for pin modes `Flashlight Digital`, 
    `Trigger Cycle Start` and `Resume WLAN Connection`.


!!! Note
    External wiring of the respective gpio pin requires to match internal signal level 
    configuration. Especially verify the external pullup or pulldown setup.


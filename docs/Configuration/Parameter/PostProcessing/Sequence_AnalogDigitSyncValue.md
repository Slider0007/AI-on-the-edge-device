# Parameter: Analog/Digit Sync Value

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Analog/Digit Sync Value | [sequence].analogdigitsyncvalue
| Default Value     | `9.2`               | `9.2`
| Input Options     | `6.0` .. `9.9`      | `6.0` .. `9.9` 


## Description

Adjusts the synchronization between the least significant digit and the most significant analog counter in a number sequence.
Check [jomjol documentation](https://jomjol.github.io/AI-on-the-edge-device-docs/Watermeter-specific-analog---digit-transition/) 
for more details.


!!! Info
    Set this slightly below the most significant analog value when the least significant digit reaches the x.8 region 
    (e.g., 0.8, 1.8). This ensures accurate rollover detection, especially on poorly synchronized mechanical dials. 
    Lower values trigger earlier digit transitions.


!!! Note
    This parameter is set individually for each number sequence. Use the dropdown to select the desired sequence.
    A number sequence includes one or more digits and/or analog counters defined in the digit or analog ROI configuration.

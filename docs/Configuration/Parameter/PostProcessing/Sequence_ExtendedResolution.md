# Parameter: Extended Resolution

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Extended Resolution | [sequence].extendedresolution
| Default Value     | `Enabled`           | `true`
| Input Options     | `Disabled`<br>`Enabled` | `false`<br>`true` 


## Description

Use the decimal place of the least-significant CNN result of the sequence to increase 
decimal place of the result by one.


!!! Note
    This parameter is only supported by `*-class*` and `*-cont*` models! 
    See [Choosing-the-Model](https://jomjol.github.io/AI-on-the-edge-device-docs/Choosing-the-Model) 
    for more details.


!!! Note
    This parameter is set individually for each number sequence. Use the dropdown to select the desired sequence.
    A number sequence includes one or more digits and/or analog counters defined in the digit or analog ROI configuration.

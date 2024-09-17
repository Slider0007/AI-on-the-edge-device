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
    This parameter can to be set for each number sequence separately. 
    Use the drop down to choose the respective number sequence. 
    A number sequence is a group of single digits and / or analog counter defined in digit 
    and / or anlog ROI configuration screen.

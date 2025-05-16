# Parameter: Ignore Leading NaNs

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Ignore Leading NaNs | [sequence].ignoreleadingnan
| Default Value     | `Disabled`          | `false`
| Input Options     | `Disabled`<br>`Enabled` | `false`<br>`true` 


## Description

Leading `N` will be deleted before further post-processing. 


!!! Note
    This is only relevant for `dig-class-11*` models or `dig-cont*` models 
    (result fit < CNN Good Threshold) which use `N` presentation! 
    See [Choosing-the-Model](https://jomjol.github.io/AI-on-the-edge-device-docs/Choosing-the-Model)
    for details.


!!! Note
    This parameter is set individually for each number sequence. Use the dropdown to select the desired sequence.
    A number sequence includes one or more digits and/or analog counters defined in the digit or analog ROI configuration.

# Parameter: Ignore Leading NaNs

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Ignore Leading NaNs | [sequence].ignoreleadingnan
| Default Value     | `Disabled`          | `false`
| Input Options     | `Disabled`<br>`Enabled` | `false`<br>`true` 


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!<br>
    For regular use cases, it is not recommended to use at all, because it can disturb post-processing. 


## Description

Leading `N`s in raw value will be deleted before further post-processing


!!! Note
    This parameter is only supported when using `dig-class-11*` models


!!! Note
    This parameter is set individually for each number sequence. Use the dropdown to select the desired sequence.
    A number sequence includes one or more digits and/or analog counters defined in the digit or analog ROI configuration.

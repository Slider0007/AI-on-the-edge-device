# Parameter: Analog/Digital Transition Start

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Analog/Digit Sync Value | [sequence].analogdigitsyncvalue
| Default Value     | `9.2`               | `9.2`
| Input Options     | `6.0` .. `9.9`      | `6.0` .. `9.9` 


## Description

Adapt the sync strategy between least-significant digit number and most-significant analog counter in a sqeuence. 
Check [documentation](../Watermeter-specific-analog---digital-transition) for more details.

!!! Info
    The lower the value, the earlier the least significant digit reaches the critical zweo crossing area. 
    Set this parameter a little lower than the most-significant analog counter value right after 
    least-significant digit number enters `x.8` region.

!!! Note
    This parameter can to be set for each number sequence separately. 
    Use the drop down to choose the respective number sequence. 
    A number sequence is a group of single digits and / or analog counter defined in digit 
    and / or anlog ROI configuration screen.

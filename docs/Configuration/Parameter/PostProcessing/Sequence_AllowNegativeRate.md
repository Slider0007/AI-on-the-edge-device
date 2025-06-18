# Parameter: Allow Negative Rate

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Allow Negative Rate | [sequence].allownegativerate
| Default Value     | `Disabled`          | `false`
| Input Options     | `Disabled`<br>`Enabled` | `false`<br>`true` 



## Description

Allow decreasing values (backwards counting).


!!! Note
    For most use cases this option should be set to `Disabled` e.g. for water or gas meters 
    (-> plausibility check can be performed to avoid negative rates). But for some use cases 
    like for e.g. pressure sensors negative rates a accepted.


!!! Note
    This parameter is set individually for each number sequence. Use the dropdown to select the desired sequence.
    A number sequence includes one or more digits and/or analog counters defined in the digit or analog ROI configuration.

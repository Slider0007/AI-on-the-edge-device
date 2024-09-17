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
    (-> plausiblity check can be performed to avoid negative rates). But for some use cases 
    like for e.g. pressure sensors negative rates a accepted.


!!! Note
    This parameter can to be set for each number sequence separately. Use the drop down to 
    choose the respective number sequence. A number sequence is a group of single digits 
    and / or analog counter defined in digit and / or anlog ROI configuration screen.

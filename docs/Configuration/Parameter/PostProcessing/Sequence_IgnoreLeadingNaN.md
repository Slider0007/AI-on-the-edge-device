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
    This parameter can to be set for each number sequence separately. 
    Use the drop down to choose the respective number sequence. 
    A number sequence is a group of single digits and / or analog counter defined in digit 
    and / or anlog ROI configuration screen.

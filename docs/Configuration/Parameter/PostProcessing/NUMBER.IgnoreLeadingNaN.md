# Parameter: Ignore Leading NaNs

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Ignore Leading NaNs | [NUMBER SEQUENCE].IgnoreLeadingNaN
| Default Value     | `false`             | `false`
| Input Options     | `false`<br>`true`   | `false`<br>`true` 


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!  


## Description

Leading `N` will be deleted before further post-processing. 


!!! Note
    This is only relevant for class-11 models or dig-cont models (result fit < CNN Good Threshold) which use `N` presentation!<br>
    See [Choosing-the-Model](https://jomjol.github.io/AI-on-the-edge-device-docs/Choosing-the-Model) for details.


!!! Note
    This parameter can to be set for each number sequence separately.
    Use the drop down to choose the respective number sequence. 
    A number sequence is a group of single digits and / or analog counter defined in digit and / or anlog ROI configuration screen.

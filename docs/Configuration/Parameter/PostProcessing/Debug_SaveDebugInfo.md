# Parameter: Save Debug Info

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Save Debug Info     | savedebuginfo
| Default Value     | `Disabled`          | `false`
| Input Options     | `Disabled`<br>`Enabled` | `false`<br>`true` 


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!


## Description

Save the following debug information in case of the follwoing deviation during post-processing: <br>
1. Deviation 'Rate too high (<)' in terms of negative rate: Save single ROI images + result file <br>
2. Deviation 'Rate too high (>)' in terms of positive rate: Save single ROI images + result file <br>
<br>
Debug path: `/log/debug/`<br>

!!! Note
    This event will be additionally logged as a warning event (process deviation).
    Process will be continued without any further action, though.

# Parameter: Use Fallback Value

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Use Fallback Value  | [sequence].UseFallbackValue
| Default Value     | `Enabled`           | `true`
| Input Options     | `Disabled`<br>`Enabled` | `false`<br>`true` 


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!


## Description

Use the fallback value (last valid result) for post-processing plausibility actions and 
checks (Negative rate, Rate too high, resubstitution of N positions in number sequence, ...).

!!! Tip
    It is **strongly recommended** to keep this parameter activated. If disabled, a less reliable 
    processing is to be expected and no rate calculation is performed.

!!! Note
    The fallback value gets backuped to NVS to prevent against power loss.

# Parameter: Use Fallback Value

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Use Fallback Value  | FallbackValueUse
| Default Value     | `true`              | `true`
| Input Options     | `false`<br>`true`   | `false`<br>`true` 


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!


## Description

Use the Fallback Value (last valid result) for post-processing some plausibility actions and checks (Negative rate, Rate too high, 
Resubstitution of N positions in number sequence).

!!! Tip
    It is **strongly recommended** to keep this parameter activated. If disabled, a less reliable processing is to be expected and no
    rate calculation is performed.

!!! Note
    The Fallback Value gets backuped to NVS to prevent against power loss.

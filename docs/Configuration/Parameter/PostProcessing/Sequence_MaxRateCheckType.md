# Parameter: Max Rate Check Type

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Max Rate Check Type | [sequence].maxratechecktype
| Default Value     | `Rate Per Minute`   | `1`
| Input Options     | `No Rate Check`<br>`Rate Per Minute`<br>`Rate Per Interval` | `0` .. `2`


## Description

This parameter defines which approach is used to check rate / value delta to avoid unrealistic 
value jumps in positive and also in negative direction.


### Input Options

| Input Option              | Description
|:---                       |:---
| `No Rate Check`           | No rate / value delta check is performed
| `Rate Per Minute`         | Delta between actual and last valid processed value (Fallback Value) and additionally normalized to a minute. -> Self-healing: Delta time as calculation base is increasing over time -> rate/min is getting lower and lower. At some point the rate is in accepatable range again.
| `Rate Per Interval`       | Delta between actual and last valid processed value (Fallback Value) -> Not self-healing: Value delta is increasing over time. Rate limit is fixed. Limit should cover realistic consumption + possible false readings.


!!! Note
    The rate / value delta is checked against positive and negative rate deviation.

!!! Note
    This parameter can to be set for each number sequence separately.
    Use the drop down to choose the respective number sequence. 
    A number sequence is a group of single digits and / or analog counter defined in digit and / or anlog ROI configuration screen.

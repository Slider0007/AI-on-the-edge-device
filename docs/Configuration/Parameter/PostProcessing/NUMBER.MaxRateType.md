# Parameter: Rate Check Type

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Rate Check Type     | [NUMBER SEQUENCE].MaxRateType
| Default Value     | `Rate Per Minute`   | `RatePerMin`
| Input Options     | `Rate Per Minute`<br>`Rate Per Interval`<br>`No Rate Check`| `RatePerMin`<br>`RatePerInterval`<br>`RateOff`


## Description

This parameter defines which approach is used to check rate / value delta to avoid unrealistic 
value jumps in positive and also in negative direction.


### Input Options

| Input Option              | Description
|:---                       |:---
| `Rate Per Minute`         | Delta between actual and last valid processed value (Fallback Value) and additionally normalized to a minute. -> Self-healing: Delta time as calculation base is increasing over time -> rate/min is getting lower and lower. At some point the rate is in accepatable range again.
| `Rate Per Interval`       | Delta between actual and last valid processed value (Fallback Value) -> Not self-healing: Value delta is increasing over time. Rate limit is fixed. Limit should cover realistic consumption + possible false readings.
| `No Rate Check`           | No rate / value delta check is performed


!!! Note
    The rate / value delta is checked against positive and negative rate deviation.

!!! Note
    This parameter can to be set for each number sequence separately.
    Use the drop down to choose the respective number sequence. 
    A number sequence is a group of single digits and / or analog counter defined in digit and / or anlog ROI configuration screen.

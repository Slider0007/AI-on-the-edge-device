# Parameter: Maximum Rate Value

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Maximum Rate Value  | [NUMBER SEQUENCE].MaxRateValue
| Default Value     | `0.1`               | `0.1`
| Input Options     | `0.0` .. &infin;    | `0.0` .. &infin; 


## Description

Maximum accepted rate (positive or negative) between fallback value (last valid reading) and actual value.
If exceeded the actual value will be rejected and fallback value is used instead.<br>
Behaviour depending on the setting of `Maximum Rate Check Type`.


!!! Example
    If negative rate is disallowed and no maximum rate value is set, one false high reading will
    lead to a period of missing measurements until the measurement reaches the previous false high
    reading. E.g. if the counter is at `600.00` and it's read incorrectly as` 610.00`, all measurements
    will be skipped until the counter reaches `610.00`. Setting the MaxRateValue to `0.1` leads to a
    rejection of all readings with a difference `> 0.1`, in this case a rejection of `610,00`.
    Be aware: The rejection also applies to correct readings with a difference `> 0.1`!


!!! Note
    This parameter can to be set for each number sequence separately.
    Use the drop down to choose the respective number sequence. 
    A number sequence is a group of single digits and / or analog counter defined in digit and / or anlog ROI configuration screen.

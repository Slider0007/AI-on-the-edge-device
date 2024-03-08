# Parameter: Rate Check Limit Value

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Rate Check Limit Value | [NUMBER SEQUENCE].MaxRateValue
| Default Value     | `0.1`               | `0.1`
| Input Options     | `0.0` .. &infin;    | `0.0` .. &infin; 


## Description

Maximum accepted rate / value delta (positive or negative) between actual value and fallback value (last valid reading). 
If exceeded the value of the actual cycle going to be rejected and fallback value is used instead.<br>
Behaviour depending on the parameter of `Rate Check Type`.


!!! Example
    If negative rate is disallowed and no rate check limit value is set, one false high reading will
    lead to a period of missing measurements until the measurement reaches the previous false high
    reading. E.g. if the counter is at `600.00` and it's read incorrectly as` 610.00`, all measurements
    will be skipped until the counter reaches `610.00`. Setting the 'Rate Check Limit Value' to `0.1` leads to a
    rejection of all readings with a difference `> 0.1`, in this case a rejection of `610,00`.
    Be aware: Correct readings `> 0.1` get also rejected!


!!! Note
    This parameter can to be set for each number sequence separately.
    Use the drop down to choose the respective number sequence. 
    A number sequence is a group of single digits and / or analog counter defined in digit and / or anlog ROI configuration screen.

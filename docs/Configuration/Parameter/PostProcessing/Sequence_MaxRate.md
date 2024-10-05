# Parameter: Max Rate

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Max Rate            | [sequence].maxrate
| Default Value     | `0.100`             | `0.100`
| Input Options     | `0.001` .. &infin;  | `0.001` .. &infin; 


## Description

Maximum accepted rate / value delta (positive or negative) between actual value and fallback value (last valid reading). 
If exceeded the value of the actual cycle going to be rejected and fallback value is used instead. 
Behaviour depending on the parameter of `Rate Check Type`.


!!! Example
    If negative rate is disallowed and no rate check limit value is set, one false high reading will
    lead to a period of missing measurements until the measurement reaches the previous false high
    reading. E.g. if the counter is at `600.00` and it's read incorrectly as` 610.00`, all measurements
    will be skipped until the counter reaches `610.00`. Setting the 'Rate Check Limit Value' to `0.100` leads to a
    rejection of all readings with a difference `> 0.100`, in this case a rejection of `610,00`.
    Be aware: Correct readings `> 0.100` get also rejected!


!!! Note
    This parameter can to be set for each number sequence separately. 
    Use the drop down to choose the respective number sequence. 
    A number sequence is a group of single digits and / or analog counter defined in digit and / 
    or anlog ROI configuration screen.

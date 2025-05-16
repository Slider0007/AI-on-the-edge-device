# Parameter: Check Digit Increase Consistency

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Check Digit Increase Consistency | [sequence].checkdigitincreaseconsistency
| Default Value     | `Disabled`          | `false`
| Input Options     | `Disabled`<br>`Enabled` | `false`<br>`true` 


## Description

An additional post-processing consistency check to improve zero crossing of rolling digit numbers.


!!! Warning
    It's not recommended to use with LCD digit numbers!<br>
    Only useable for `dig-class11` models (0-9 + NaN).


!!! Note
    This parameter is set individually for each number sequence. Use the dropdown to select the desired sequence.
    A number sequence includes one or more digits and/or analog counters defined in the digit or analog ROI configuration.
    

# Parameter: Check Digit Increase Consistency

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Check Digit Increase Consistency | [sequence].checkdigitincreaseconsistency
| Default Value     | `Disabled`          | `false`
| Input Options     | `Disabled`<br>`Enabled` | `false`<br>`true` 


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!<br>


## Description

A post-processing consistency check to improve zero crossing detection of **rolling digits**.


!!! Warning
    It's not recommended at all to use with LCD digits.<br>
    It's recommended to use alternative models than `dig-class11` for rolling digit processing.


!!! Note
    Only useable for `dig-class11` models (0-9 + NaN).


!!! Note
    This parameter is set individually for each number sequence. Use the dropdown to select the desired sequence.
    A number sequence includes one or more digits and/or analog counters defined in the digit or analog ROI configuration.
    

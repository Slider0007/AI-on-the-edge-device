# Parameter: Check Digit Increase Consistency

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Check Digit Increase Consistency | [sequence].checkdigitincreaseconsistency
| Default Value     | `Disabled`          | `false`
| Input Options     | `Disabled`<br>`Enabled` | `false`<br>`true` 


## Description

An additional post-processing consistency check to improve zero crossing of rolling digit numbers.


!!! Warning
    It's not recommended to use with LCD digit numbers!


!!! Note
    Only useable for class-11 (0-9 + NaN) models.
    

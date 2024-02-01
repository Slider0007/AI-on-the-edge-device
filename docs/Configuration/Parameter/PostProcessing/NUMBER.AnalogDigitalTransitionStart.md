# Parameter: Analog/Digital Transition Start

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Analog/Digital Transition Start | [NUMBER SEQUENCE].AnalogDigitalTransitionStart
| Default Value     | `9.2`               | `9.2`
| Input Options     | `6.0` .. `9.9`      | `6.0` .. `9.9` 


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!  


## Description

This can be used if you have wrong values, but the recognition of the individual ROIs are correct.
Look for the start of changing of the first digit and note the analog pointer value behind.
Set it here. Only used on combination of digits and analog pointers.
Check [documentation](../Watermeter-specific-analog---digital-transition) for more details.


!!! Note
    This parameter can to be set for each number sequence separately.
    Use the drop down to choose the respective number sequence. 
    A number sequence is a group of single digits and / or analog counter defined in digit and / or anlog ROI configuration screen.

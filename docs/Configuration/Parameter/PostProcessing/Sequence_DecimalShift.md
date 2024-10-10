# Parameter: Decimal Shift

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Decimal Shift       | [sequence].decimalshift
| Default Value     | `0`                 | `0`
| Input Options     | `-9` .. `9`         | `-9` .. `9`


## Description

Shift the decimal separator (positive or negative).<br>
A positive shift of one (1) increase the result by factor 10, a positive shift of two (2) 
increase the result by factor 100, ...<br> A negative shift of one (1) decrease the result 
by factor 10, a negative shift of two (2) increase the result by factor 100, ... 
-> E.g. To get a `liter` output from a `m³` reading  (`1 m³` -> `1000 liters`), a decimal 
shift of 3 needs to be set.


!!! Note
    This parameter can to be set for each number sequence separately. 
    Use the drop down to choose the respective number sequence. 
    A number sequence is a group of single digits and / or analog counter defined in 
    digit and / or anlog ROI configuration screen.

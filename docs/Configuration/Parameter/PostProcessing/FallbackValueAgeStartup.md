# Parameter: Max. Age of Fallback Value

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Max. Age of Fallback Value | FallbackValueAgeStartup
| Default Value     | `60`                | `60`
| Input Options     | `0` .. &infin;      | `0` .. &infin; 
| Unit              | Minutes             | Minutes


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!  


## Description

Defines the maximum age of the Fallback Value (last valid result) calculated from last backup timestamp to actual 
value reload timestamp after device boot.


!!! Note
    This helps to prevent using a not up-to-date last valid result after e.g. a longer period being offline.
    A too old last valid result (Fallback Value) will be updated with first new evaluated result.

# Parameter: Automatic Process Interval

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Automatic Process Interval | automaticprocessinterval
| Default Value     | `2.0`               | `2.0`
| Input Options     | `0.1` .. &infin;    | `0.1` .. &infin;
| Unit              | Minutes             | Minutes


## Description

Interval in which the process (digitization cycle) is periodically started (Operation Mode: Automatic)


!!! Note
    If processing of a 'digitization cycle' takes longer than the configured process interval, 
    the scheduled cycle gets only started after previous cycle is completed.

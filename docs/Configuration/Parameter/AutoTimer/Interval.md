# Parameter: Processing Interval

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Processing Interval | Interval
| Default Value     | `5.0`               | `5.0`
| Input Options     | `1.0` .. &infin;    | `1.0` .. &infin;
| Unit              | Minutes             | Minutes


## Description

Interval in which the process (digitization round) is started automatically (if 'Automatic Process Start' is configured).

!!! Note
    If processing of a 'digitization round' takes longer than this interval, the next process gets only started when previous is completed.

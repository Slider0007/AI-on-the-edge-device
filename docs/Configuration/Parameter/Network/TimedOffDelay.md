# Parameter: Timed-Off Delay

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Timed-Off Delay     | timedoffdelay
| Default Value     | `60`                | `60`
| Input Range       | `1` .. &infin;      | `1` .. &infin;
| Unit              | Minutes             | Minutes  

## Description

Define the delay after which the network shall be suspended.<br>
- WLAN Client: Suspend after the defined time is elapsed and actual processed cycle is completed
- Access Point: Suspend after the defined time with no client connected is elapsed and actual processed cycle is completed

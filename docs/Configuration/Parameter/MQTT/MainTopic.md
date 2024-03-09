# Parameter: Main Topic

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Main Topic          | MainTopic
| Default Value     | `watermeter`        | `watermeter`


## Description

MQTT main topic where the data are published.<br>
Check MQTT API description for more details.

!!! Note
    The results are pubished per number sequence: `[Main Topic]/[Number Sequence]/[Topic]`.<br>
    A number sequence is a group of single digits and / or analog counter defined in digit and / or anlog ROI configuration screen.

!!! Note
    The general connection status gets published to `[Main Topic]/connection`. 

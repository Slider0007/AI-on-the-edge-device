# Parameter: Main Topic

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Main Topic          | maintopic
| Default Value     | `watermeter`        | `watermeter`


## Description

MQTT main topic where the data are published. 
Nested topic is supported. Check MQTT API description for more details.

!!! Note
    The results are pubished per number sequence: `[Main Topic]/process/data/[Number Sequence ID]/...`. 
    A number sequence is a group of single digits and / or analog counter defined in digit and / 
    or anlog ROI configuration screen.

!!! Note
    The general connection status gets published to `[Main Topic]/device/status/connection`. 

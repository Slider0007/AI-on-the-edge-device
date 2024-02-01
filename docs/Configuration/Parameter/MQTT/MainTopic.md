# Parameter: Main Topic

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Main Topic          | MainTopic
| Default Value     | `watermeter`        | `watermeter`


## Description

MQTT main topic where the data are published.

!!! Note
    The results are pubished per number sequence, e.g. `MainTopic/[NUMBER SEQUENCE]/[RESULT TOPIC]`.
    A number sequence is a group of single digits and / or analog counter defined in digit and / or anlog ROI configuration screen.<br>
    See [MQTT Result Topics](https://jomjol.github.io/AI-on-the-edge-device-docs/MQTT-API/#Result) for a full list of result topics.

!!! Note
    The general connection status gets published to `MainTopic/connection`. 

## Overview: MQTT API

The device is capable to register to a MQTT broker to publish data and subscribe to specific topics.

!!! __Note__: 
    Only MQTT v3.1.1 is supported.

### Parametrization
The MQTT service has to be enabled and configured properly via web interface (`Settings` > `Configuration` > section `MQTT`). 
The following parameter are minimum required to use MQTT API: : `URI`, `Main Topic`, `Client ID`


### Main Topic

All data gets located under the `MainTopic` which is defined in device configuration. The main topic can be flat 
`watermeter` or even be nested multiple times, e.g. `water/mainwatermeter` or `water/building1/meter1/...`.


### Available MQTT API topic cluster

Further details can be found in the respective MQTT API topic cluster description.

| MQTT API Topic Cluster               | Description                                      | JSON / Topics | Depre-<br>cated*       
|:-------------------------------------|:-------------------------------------------------|:------------- |:-----------
| [Device Info](device-info.md)        | Device Info (Static topic content)               | JSON          | 
| [Device Status](device-status.md)    | Device Status (Variable topic content)           | Topics        | 
| [Process Control](process-control.md)| Process Control (Topics to control process)      | Topics        | 
| [Process Data](process-data.md)      | Process Data (Number sequence data / results)    | JSON + Topics | 
| [Process Status](process-status.md)  | Process Status (Variable topic content)          | Topics        | 
| [Home Assistant](home-assistant-discovery.md) | Home Assistant Discovery Topics         | JSON          | 
| [GPIO Control / Status](gpio.md)     | General Purpose Input / Output Control / Status  | Topcis        | 

*MQTT topics which are marked as deprecated will be completely removed (functionality merged in another endpoint) or 
modified in upcoming major release. Check changelog for breaking changes.

### Migration notes (Moved or updated topics)
Check migration notes for migrated MQTT topics: [Migration notes](xxx_migration_notes.md)
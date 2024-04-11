[Overview](_OVERVIEW.md)

## Migration notes

### Migration from v16.x to v17.x

Mapping to which cluster renamed or moved topics are related to:

| MQTT API Topic Cluster               | Description                                      | JSON / Topics | Replacement for topic     
|:-------------------------------------|:-------------------------------------------------|:------------- |:-----------
| [Device Info](device-info.md)        | Device Info (Static topic content)               | JSON          | `hostname`, `IP`, `MAC`
| [Device Status](device-status.md)    | Device Status (Variable topic content)           | Topics        | `connection`, `uptime`, `freeMem`, `wifiRRSI`, `CPUtemp`
| [Process Control](process-control.md)| Process Control (Topics to control process)      | Topics        | `flow_start`, `set_fallbackvalue`
| [Process Data](process-data.md)      | Process Data (Number sequence data / results)    | JSON + Topics | Topic cluster of each sequence, e.g `main/actual_value`
| [Process Status](process-status.md)  | Process Status (Variable topic content)          | Topics        | `status`, `interval`, `process_error`
| [Home Assistant](home-assistant-discovery.md) | Home Assistant Discovery Topics         | JSON          | 
| [GPIO Control / Status](gpio.md)     | General Purpose Input / Output Control / Status  | Topcis        | Topic cluster of `GPIO`
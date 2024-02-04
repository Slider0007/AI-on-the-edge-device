## Overview: REST API

### General usage
- Generic: `http://IP-ADDRESS/REST API endpoint`
- Example: `http://192.168.1.x/process_data`

Further details can be found in the respective REST API endpoint description.

| REST API Endpoint                    | Description                                        | HTML / JSON | Depre-<br>cated*       
|:-------------------------------------|:---------------------------------------------------|:------------|:-----------
| [/process_data](process_data.md)     | Process Data                                       | JSON        | 
| [/value](value.md)                   | Process Data                                       | HTML        | X
| [/json](json.md)                     | Process Data                                       | JSON        | X
| [/info](info.md)                     | Device/Process Info                                | HTML        | 
| [/sysinfo](sysinfo.md)               | Device/Process Info                                | JSON        | X
| [/statusflow](statusflow.md)         | Flow Prozess State                                 | HTML        | X
| [/process_error](process_error.md)   | Flow Process Error                                 | HTML        | X
| [/starttime](starttime.md)           | Device Boot Time                                   | HTML        | X
| [/cpu_temperature](cpu_temperature.md)| CPU Temperature                                   | HTML        | X
| [/rssi](rssi.md)                     | WLAN Signal Strength                               | HTML        | X
| [/heap](heap.md)                     | Heap Info                                          | HTML        | X
| [/flow_start](flow_start.md)         | Trigger Cycle (Flow) Start                         | HTML        | X
| [/reload_config](reload_config.md)   | Reload Configuration                               | HTML        | 
| [/set_fallbackvalue](set_fallbackvalue.md) | Set Fallback Value                           | HTML        | 
| [/editflow](editflow.md)             | Parametrization Helper                             | HTML        | 
| [/lighton](lighton.md)               | Switch onboard flashlight on                       | HTML        | X
| [/lightoff](lightoff.md)             | Switch onboard flashlight off                      | HTML        | X
| [/capture](capture.md)               | Capture image (without flashlight)                 | HTML        | 
| [/capture_with_flashlight](capture_with_flashlight.md) | Capture image with flashlight    | HTML        | X
| [/save](caputure_save.md)            | Save a new image to SD card                        | HTML        | X
| [/GPIO](gpio.md)                     | Read / Control GPIO                                | HTML        | 
| [/mqtt_publish_discovery](mqtt_publish_discovery.md)|Publish Home Assistant MQTT Discovery Topics| HTML | 
| [/datafileact](datafileact.md)       | Data of today                                      | HTML        | X
| [/data](data.md)                     | Data of today (last 80kB)                          | HTML        | 
| [/logfileact](logfileact.md)         | Log of today                                       | HTML        | X
| [/log](log.md)                       | Log of today (last 80kB)                           | HTML        | 
| [/stream](stream.md)                 | Camera Live Stream                                 | HTML        | 
| [/ota](ota.md)                       | Over The Air Update                                | HTML        | 
| [/reboot](reboot.md)                 | Trigger Reboot                                     | HTML        | 
| [/fileserver/](fileserver.md)        | Fileserver                                         | HTML        | 
| [/upload/](upload.md)                | File Upload (POST)                                 | HTML        | 
| [/delete/](delete.md)                | File Deletion (POST)                               | HTML        | 
| [/img_tmp/](img_tmp.md)              | Load Images From RAM                               | HTML        | 
| /                                    | WebUI (Redirected to `index.html`)                 | HTML        | 


*Endpoints which are marked as deprecated will be completely removed (functionality merged in another endpoint) or modified in upcoming major release. Check changelog for breaking changes.
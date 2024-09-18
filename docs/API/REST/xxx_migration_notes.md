[Overview](_OVERVIEW.md)

## Migration notes

### Migration from v16.x to v17.x

Mapping which new endpoint replaces functionality of removed endpoint:

| REST API Endpoint                    | Description                                        | HTML / JSON | Replacement for removed REST APT Endpoint      
|:-------------------------------------|:---------------------------------------------------|:------------|:-----------
| [/process_data](process_data.md)     | Process Data                                       | JSON + HTML | `/json`, `/value` 
| [/info](info.md)                     | Device Info + Process Status                       | JSON + HTML | `/starttime`, `/uptime`, `/rssi`, `/sysinfo`, `/cpu_temperature`, `/heap` 
| [/config](config.md)                 | Device Configuration                               | JSON + HTML | `/reload_config`
| [/cycle_start](cycle_start.md)       | Trigger Cycle (Flow) Start                         | HTML        | 
| [/set_fallbackvalue](set_fallbackvalue.md) | Set Fallback Value                           | HTML        | 
| [/editflow](editflow.md)             | Parametrization Helper                             | HTML        |
| [/camera](camera.md)                 | Camera Capture, Stream, Parametrization + Flashlight| HTML       | `/editflow?task=test_take`, `/capture`, `/capture_with_flashlight`, `/save`, `/lighton`, `/lightoff`, `/stream`
| [/gpio](gpio.md)                     | Read / Control GPIO                                | HTML        | `/GPIO`
| [/mqtt](mqtt.md)                     | Publish HA discovery topics / Device info topics   | HTML        | `/mqtt_publish_discovery`
| [/data](data.md)                     | Data of today (last 80kB)                          | HTML        | `/datafileact`
| [/log](log.md)                       | Log of today (last 80kB)                           | HTML        | `/logfileact`
| [/ota](ota.md)                       | Over The Air Update                                | HTML        | 
| [/reboot](reboot.md)                 | Trigger Reboot                                     | HTML        | 
| [/fileserver/](fileserver.md)        | Fileserver                                         | HTML        | 
| [/upload/](upload.md)                | File Upload (POST)                                 | HTML        | 
| [/delete/](delete.md)                | File Deletion (POST)                               | HTML        | 
| [/img_tmp/](img_tmp.md)              | Load Images From RAM                               | HTML        | 
| /                                    | WebUI (Redirected to `index.html`)                 | HTML        | 

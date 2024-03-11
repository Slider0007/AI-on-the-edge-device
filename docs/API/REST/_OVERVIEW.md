## Overview: REST API

### General usage
- Generic: `http://IP-ADDRESS/REST API endpoint`
- Example: `http://192.168.1.x/process_data`

### Available REST API endpoints

Further details can be found in the respective REST API endpoint description.

| REST API Endpoint                    | Description                                        | HTML / JSON | Depre-<br>cated*       
|:-------------------------------------|:---------------------------------------------------|:------------|:-----------
| [/process_data](process_data.md)     | Process Data                                       | JSON + HTML | 
| [/info](info.md)                     | Device Info + Process Status                       | JSON + HTML | 
| [/cycle_start](cycle_start.md)       | Trigger Cycle (Flow) Start                         | HTML        | 
| [/reload_config](reload_config.md)   | Reload Configuration                               | HTML        | 
| [/set_fallbackvalue](set_fallbackvalue.md) | Set Fallback Value                           | HTML        | 
| [/editflow](editflow.md)             | Parametrization Helper                             | HTML        | 
| [/recognition_details](recognition_details.md)|Image Recognition Details (WebUI Page)     | HTML        |
| [/camera](camera.md)                 | Camera Capture, Stream, Parametrization + Flashlight| HTML       | 
| [/gpio](gpio.md)                     | Read / Control GPIO                                | HTML        | 
| [/mqtt](mqtt.md)                     | Publish HA discovery topics / Device info topics   | HTML        | 
| [/data](data.md)                     | Data of today (last 80kB)                          | HTML        | 
| [/log](log.md)                       | Log of today (last 80kB)                           | HTML        | 
| [/ota](ota.md)                       | Over The Air Update                                | HTML        | 
| [/reboot](reboot.md)                 | Trigger Reboot                                     | HTML        | 
| [/fileserver/](fileserver.md)        | Fileserver                                         | HTML        | 
| [/upload/](upload.md)                | File Upload (POST)                                 | HTML        | 
| [/delete/](delete.md)                | File Deletion (POST)                               | HTML        | 
| [/img_tmp/](img_tmp.md)              | Load Images From RAM                               | HTML        | 
| /                                    | WebUI (Redirected to `index.html`)                 | HTML        | 


*Endpoints which are marked as deprecated will be completely removed (functionality merged in another endpoint) or 
modified in upcoming major release. Check changelog for breaking changes.

### Migration notes (Removed / updated endpoints)
Check migration notes for migrated or removed REST API endpoints: [Migration notes](xxx_migration_notes.md)

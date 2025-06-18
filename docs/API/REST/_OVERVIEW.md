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
| [/config](config.md)                 | Device Configuration                               | JSON + HTML | 
| [/metrics](metrics.md)               | Prometheus / OpenMetrics Data                      | HTML        | 
| [/cycle_start](cycle_start.md)       | Trigger Cycle (Flow) Start                         | HTML        | 
| [/reload_config](reload_config.md)   | Reload Configuration                               | HTML        | 
| [/set_fallbackvalue](set_fallbackvalue.md) | Set Fallback Value                           | HTML        | 
| [/editflow](editflow.md)             | Parametrization Helper                             | HTML        | 
| [/recognition_details](recognition_details.md)|Image Recognition Details (WebUI Page)     | HTML        |
| [/camera](camera.md)                 | Camera Capture, Stream, Parametrization + Flashlight| HTML       | 
| [/gpio](gpio.md)                     | Read / Control GPIO                                | JSON + HTML | 
| [/mqtt](mqtt.md)                     | Publish HA discovery topics / Device info topics   | HTML        | 
| [/data](data.md)                     | Data of today (last 80kB)                          | HTML        | 
| [/log](log.md)                       | Log of today (last 80kB)                           | HTML        | 
| [/ota](ota.md)                       | Over The Air Update                                | HTML        | 
| [/reboot](reboot.md)                 | Trigger Reboot                                     | HTML        | 
| [/wlan](wlan.md)                     | WLAN Scan                                          | JSON        |
| [/coredump](coredump.md)             | Handle Core Dumps (Software Exception)             | HTML        | 
| [/fileserver/](fileserver.md)        | Fileserver                                         | HTML        | 
| [/upload/](upload.md)                | File Upload (POST)                                 | HTML        | 
| [/delete/](delete.md)                | File Deletion (POST)                               | HTML        | 
| [/img_tmp/](img_tmp.md)              | Load Images From RAM                               | HTML        | 
| /                                    | WebUI (Redirected to `index.html` or `setup.html`) | HTML        | 


*Endpoints which are marked as deprecated will be completely removed (functionality merged in another endpoint) or 
modified in upcoming major release. Check changelog for breaking changes.


### REST API Endpoint Security
All REST API endpoints can be (limited) secured with basic HTTP authentication scheme. The endpoint authentication 
has to be configured via WebUI: `Settings > Configuration > Section 'WebUI' > Authentication`.

#### Security Considerations
  - Only basic access authentication scheme using unencrypted HTTP protocol is implemented. 
  - The basic authentication scheme is not a secure method of user authentication, nor does it in any way 
    protect the entity, which is transmitted in cleartext (only Base64 encoded, not encrypted or hashed) 
    across the physical network used as the carrier.
  - All data (also security related data, e.g. password) are transmitted unencrypted (HTTP only, no HTTPS).

#### Usage
1. Include `Authorization` header to REST API endpoint request
    - Header value: `Basic ` concatenated with Base64 encoded `USERNAME:PASSWORD`, e.g. `Basic abcdefghijklnm=`
2. `http://{USERNAME}:{PASSWORD}@{IP-ADDRESS}/{REST API Endpoint}`, e.g. `http://username:password@192.168.4.1/info`

#### Endpoint Response
If authorization is enabled and `Authorization` header is missing or authorization is rejected, system is responding 
with status `401 Unauthorized`. Check REST API response message to get more details / rejection reason.


### Migration notes (Removed / updated endpoints)
Check migration notes for migrated or removed REST API endpoints: [Migration notes](xxx_migration_notes.md)

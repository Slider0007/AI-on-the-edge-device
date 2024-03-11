# Parameter: Home Assistant Status Topic	

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Home Assistant Status Topic | HAStatusTopic	
| Default Value     | `homeassistant/status` | `homeassistant/status`


## Description

Define Home Assistant status topic.<br>
The Home Assistant status topic signals the status of Home Assistant instance (`online` / `offline`). 
This signal is used to publish discovery topics -> Refer to parameter description of `Enable Discovery`.

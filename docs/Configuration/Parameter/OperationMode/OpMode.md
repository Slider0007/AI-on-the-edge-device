# Parameter: Operation Mode

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Operation Mode | [sequence].maxratechecktype
| Default Value     | `Automatic`         | `1`
| Input Options     | `Setup`<br>`Manual`<br>`Automatic` | `-1`<br>`0`<br>`1`


## Description

Define the process operation mode


### Input Options

| Input Option     | Description
|:---              |:---
| `Setup`          | Redirect to initial setup procedure (/setup.html)
| `Manual`         | Cycle start is only be triggerd by manual request on WebUI or external trigger (REST, MQTT, GPIO)
| `Automatic`      | Automatic cycle start by a periodic scheduled trigger with interval defined with parameter `Automatic Process Interval`

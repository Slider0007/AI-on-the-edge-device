# Parameter: Retain Process Data

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Retain Process Data  RetainProcessData
| Default Value     | `Disabled`          | `false`
| Input Options     | `Disabled`<br>`Enabled` | `false`<br>`true` 


## Description

Enable or disable [message retain flag](https://www.hivemq.com/blog/mqtt-essentials-part-8-retained-messages/)
for process related data -> `[Main Topic]/process/data/...`.

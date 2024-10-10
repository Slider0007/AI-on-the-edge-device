# Parameter: Data Log

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Data Log            | enabled
| Default Value     | `Disabled`          | `false`
| Input Options     | `Disabled`<br>`Enabled` | `false`<br>`true` 


## Description

Activate data logging of the results to the SD-Card. The data will be stored in 
`/log/data/data_YYYY-MM-DD.csv`. 
Check [`documentation`](https://jomjol.github.io/AI-on-the-edge-device-docs/data-logging) 
for more details.


!!! Warning
    A SD-Card has limited write cycles.
    Since the device does not do [Wear Leveling](https://en.wikipedia.org/wiki/Wear_leveling), 
    this can wear out your SD-Card!

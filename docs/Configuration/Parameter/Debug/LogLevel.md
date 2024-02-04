# Parameter: Log File Log Level

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Log File Log Level   | LogLevel
| Default Value     | `WARNING`           | `2`
| Input Options     | `ERROR`<br>`WARNING`<br>`INFO`<br>`DEBUG` | `1`<br>`2`<br>`3`<br>`4`


## Description

Set the log level of system log written to SD-Card. Set level higher for more verbose logging.


!!! Warning
    `DEBUG` or `INFO` might damage the SD-Card if enabled long term due to excessive writes to the SD-Card!
    A SD-Card has limited write cycles. Since the device does not do [Wear Leveling](https://en.wikipedia.org/wiki/Wear_leveling), this can wear out your SD-Card!

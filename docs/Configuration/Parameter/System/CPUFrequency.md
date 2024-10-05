# Parameter: CPU Frequency

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | CPU Frequency       | cpufrequency
| Default Value     | `160`               | `160`
| Input Options     | `160`<br>`240`      | `160`<br>`240` 
| Unit              | Mhz                 | Mhz


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!  


## Description

Set the CPU frequency.


!!! Note
    To apply this parameter a device reboot is required.


!!! Warning
    Board ESP32CAM: **It is strongly recommended to use 160Mhz**<br>
    Setting it to 240Mhz will lead to a faster processing, but it requests a stronger power supply.
    Biggest drawback: Depending on the manufacturing quality of your device, system might generally run more
    unstable and WIFI performance could be bad / unstable.

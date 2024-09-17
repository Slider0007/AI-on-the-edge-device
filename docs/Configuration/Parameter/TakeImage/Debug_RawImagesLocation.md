# Parameter: Raw Images Location

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Raw Images Location | rawimageslocation
| Default Value     | `/log/source`       | `/log/source`


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!


## Description

Folder on SD-Card where the raw image files are stored


!!! Warning
    This image file saving function is disabled by default. A SD-Card has limited write cycles. 
    Since the device does not do [Wear Leveling](https://en.wikipedia.org/wiki/Wear_leveling), 
    this can wear out your SD-Card!

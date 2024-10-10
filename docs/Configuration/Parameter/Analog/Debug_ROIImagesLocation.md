# Parameter: ROI Images Location

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | ROI Images Location | roiimageslocation
| Default Value     | `/log/analog`       | `/log/analog`


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!


## Description

Folder on SD-Card where the ROI image files are stored.


!!! Warning
        This image file saving function is disabled by default. A SD-Card has limited write cycles. 
        Since the device does not do [Wear Leveling](https://en.wikipedia.org/wiki/Wear_leveling), 
        this can wear out your SD-Card!

# Parameter: Alignment Algorithm

|                   | WebUI                   | REST API
|:---               |:---                     |:----
| Parameter Name    | Alignment Algorithm     | alignmentalgo
| Default Value     | `Rotate + Align (SAD:1CH)` | `0`
| Input Options     | `Rotate + Align (SAD:1CH)`<br>`Rotate + Align (SAD:3CH)`<br>`Rotation Only`<br>`Off` | `0`<br>`1`<br>`3`<br>`4`


## Description

Image alignment is done in two steps:  
1. Rotate image by `Image Rotation` angle
2. Use matching algorithm to find best pattern match for the alignment marker image in defined search area (`SearchFieldX`, `SearchFieldY`)


### Input Options

| Input Option               | Description
|:---                        |:---
| `Rotate + Align (SAD:1CH)` | Rotate image + Process SAD (Sum of Absolute Difference) algorithm (only red color channel)
| `Rotate + Align (SAD:3CH)` | Rotate image + Process SAD (Sum of Absolute Difference) algorithm (3 color channels: higher accuracy, but slower)
| `Rotation Only`            | Rotate image by defined rotation angle
| `Off`                      | Disable image rotation and alignment

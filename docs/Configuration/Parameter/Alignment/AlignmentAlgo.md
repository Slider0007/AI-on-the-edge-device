# Parameter: Alignment Algorithm

|                   | WebUI                   | Config.ini
|:---               |:---                     |:----
| Parameter Name    | Alignment Algorithm     | AlignmentAlgo
| Default Value     | `Rotation + Default Algo` | `Default`
| Input Options     | `Rotation + Default Algo`<br>`Rotation + High Accuracy Algo`<br>`Rotation + Fast Algo`<br>`Only Rotation`<br>`Off` | `Default`<br>`HighAccuracy`<br>`Fast`<br>`Rotation`<br>`Off`


## Description

Image alignment is done in three steps during 'Image Alignment' state:  
1. Flip image size and/or mirror image
2. Rotate image using 'Image Rotation' parameter
3. Use SAD algorithm to find pattern matches for the given alignment marker images


### Input Options

| Input Option              | Description
|:---                       |:---
| `Rotation + Default Algo` | Rotate image + Process SAD algorithm (only red color channel)
| `Rotation + High Accuracy Algo` | Rotate image + Process SAD algorithm (all 3 color channels -> 3x slower)
| `Rotation + Fast Algo`    | Rotate image + Use last working correction (If pattern still matching; Fallback: Default Algo)
| `Only Rotation`           | Only rotate image
| `Off`                     | Disable image rotation and SAD algorithm processing

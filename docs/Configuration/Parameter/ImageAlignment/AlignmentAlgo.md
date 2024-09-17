# Parameter: Alignment Algorithm

|                   | WebUI                   | REST API
|:---               |:---                     |:----
| Parameter Name    | Alignment Algorithm     | alignmentalgo
| Default Value     | `Rotation + Default Algo` | `0`
| Input Options     | `Rotation + Default Algo`<br>`Rotation + High Accuracy Algo`<br>`Rotation + Fast Algo`<br>`Only Rotation`<br>`Off` | `0`<br>`1`<br>`2`<br>`3`<br>`4`


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

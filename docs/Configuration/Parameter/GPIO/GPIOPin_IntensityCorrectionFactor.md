# Parameter: LED Intensity Correction Factor

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | LED Intensity Correction Factor | intensitycorrectionfactor
| Default Value     | `100`               | `100`
| Input Options     | `1 .. 100`          | `1 .. 100`
| Unit              | %                   | %



## Description

Apply output specific intensity correction factor (only for flashlight modes). 
This potentially helps to align different intensity levels if multiple LED types are in use.


!!! Note
    Generally, flashlight intensity is controlled with global `Flash Intensity` parameter in 
    section `Take Image`. Formula: Resulting intensity = Global `Flash Intensity` * 
    Output specific `LED Intensity Correction Factor`
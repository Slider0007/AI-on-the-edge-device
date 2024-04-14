# Parameter: LED Intensity Correction

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | LED Intensity Correction  | IOx: 12. parameter
| Default Value     | `100`               | `100`
| Input Options     | `1 .. 100`          | `1 .. 100`
| Unit              | %                   | %



## Description

Apply output specific intensity correction factor (only for flashlight modes). 
This potentially helps to align different intensity levels if multiple LED types are in use.


!!! Note
    Generally, flashlight intensity is controlled with global `Flash Intensity` parameter in 
    section `Take Image`.<br>
    Formula: Resulting intensity = Global `Flash Intensity` * Output specific `LED Intensity Correction`
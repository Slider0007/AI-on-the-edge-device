# Parameter: LED Color

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | LED Color           | LEDColor
| Default Value     | Red: `150`<br>Green: `150`<br>Blue: `150` | `150 150 150`
| Input Options     | Red: `0` (0%) .. `255` (100%)<br>Green: `0` (0%) .. `255` (100%)<br>Blue: `0` (0%) .. `255` (100%) | Red: `0` .. `255`<br>Green: `0` .. `255`<br>Blue: `0` .. `255`


## Description

Set color of the external LEDs connected to GPIO12.


!!! Note
    In parameter group GPIO12, configuartion needs to be set to `external flash light ws281x controlled`.

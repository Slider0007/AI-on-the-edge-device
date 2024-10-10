# Parameter: LED Color

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | LED Color           | colorredchannel, colorgreenchannel, colorbluechannel
| Default Value     | R: `255`<br>G: `255`<br>B: `255` | `255`, `255`, `255`
| Input Options     | R: `0` .. `255`<br>G: `0` .. `255`<br>B: `0` .. `255` | Red: `0` .. `255`<br>Green: `0` .. `255`<br>Blue: `0` .. `255`


## Description

Define desired color of the smart LEDs (RGB: additive color model)


!!! Tip
    White color: Set all three components to 255 (Red: 255 | Green: 255 | Blue: 255)


!!! Tip
    Adjust to the desired color only. Intensity adjustments can be controlled by 
    parameter `Take Image -> Flash Intensity` and `LED Intensity Correction Factor`.

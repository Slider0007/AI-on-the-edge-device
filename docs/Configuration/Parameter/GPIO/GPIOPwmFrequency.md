# Parameter: GPIO PWM Frequency

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | GPIO PWM Frequency  | IOx: 4. parameter
| Default Value     | `5000`              | `5000`
| Input Options     | `5 .. 1000000`      | `5 .. 1000000`
| Unit              | Hertz               | Hertz



## Description

GPIO PWM frequency (only for PWM controlled GPIO modes)


!!! __Note__
    Maximum duty resolution is derived from configured PWM frequency, e.g. 5Khz PWM frequency -> 13Bit<br>
    - Formula: $\log2(APBCLK Frequency / Desired Frequency) = log2(80000000 / 5000) = 13.966$<br>
    - Maximum resolution is limited to 14Bit due to compability reasons (e.g. ESP32S3)
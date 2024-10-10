# Parameter: Pin PWM Frequency

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Pin PWM Frequency   | pwmfrequency
| Default Value     | `5000`              | `5000`
| Input Options     | `5 .. 1000000`      | `5 .. 1000000`
| Unit              | Hertz               | Hertz



## Description

GPIO PWM frequency (only for PWM controlled GPIO modes)


!!! Note
    Maximum duty resolution is derived from configured PWM frequency (e.g. 5Khz PWM frequency -> 13 Bit)<br>
    - Formula: log2(APB CLK Frequency / Desired Frequency) = log2(80000000 / 5000) = 13.966<br>
    - Maximum resolution is limited to 14 Bit due to compability reasons (e.g. ESP32S3)
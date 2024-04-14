# Parameter: GPIO Expose To MQTT

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | GPIO Expose To MQTT | IOx: 5. parameter
| Default Value     | `false`             | `false`
| Input Options     | `false`<br>`true`   | `false`<br>`true` 


## Description

Enable publishing GPIO state to MQTT broker<br>
- GPIO pin state (and PWM duty)gets published to GPIO specific topic.<br>
- GPIO output / GPIO Output PWM are controlable via GPIO specific topic.


!!! Tip
    Overview of MQTT topics -> see MQTT API description


!!! Note
    MQTT functionality has to be enabled -> see section `MQTT`
# Parameter: Pin Expose To MQTT

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Pin Expose To MQTT  | exposetomqtt
| Default Value     | `Disabled`          | `false`
| Input Options     | `Disabled`<br>`Enabled` | `false`<br>`true` 


## Description

Enable publishing GPIO state to MQTT broker<br>
- GPIO pin state (and PWM duty)gets published to GPIO specific topic.<br>
- GPIO output / GPIO Output PWM are controlable via GPIO specific topic.


!!! Tip
    Overview of MQTT topics -> see MQTT API description


!!! Note
    MQTT functionality has to be enabled -> see section `MQTT`
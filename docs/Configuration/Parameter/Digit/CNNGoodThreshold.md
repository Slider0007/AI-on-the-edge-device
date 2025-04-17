# Parameter: CNN Good Threshold

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | CNN Good Threshold  | cnngoodthreshold
| Default Value     | `0.00`              | `0.00`
| Input Options     | `0.00` .. `1.00`    | `0.00` .. `1.00`


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!


## Description

Threshold above which the CNN result gets accepted.<br>
Below this threshold CNN result gets rejected for further processing.


!!! Note
    This parameter is only activated for `dig-cont-*.tflite` models! 
    `1.00` represents a 100% match. Best suitable value needs to be evaluated over a 
    longer test period. If it's rejecting the value to often, lower the threshold 
    until you find the sweet spot of recognition.

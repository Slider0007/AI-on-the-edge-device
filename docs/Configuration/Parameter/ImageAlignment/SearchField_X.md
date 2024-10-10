# Parameter: Search Field X

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Search Field X      | searchfield.x
| Default Value     | `20`                | `20`
| Input Range       | `1` .. &infin;      | `1` .. &infin;
| Unit              | Pixel               | Pixel  


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!  


## Description

Target position top left of alignment marker set on "Alignment Marker" configuration 
page +/- Search Field X (pixels in X direction): Area where the alignment pattern 
will be evaluated.  


!!! Note
     Since the alignment is one of the steps using a lot of computation time, 
     the search field should be as small as possible.
     The calculation time goes quadratic with the search field size.

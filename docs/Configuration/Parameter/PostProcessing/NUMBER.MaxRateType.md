# Parameter: Maximum Rate Check Type

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Maximum Rate Check Type | [NUMBER SEQUENCE].MaxRateType
| Default Value     | `Rate Per Minute`   | `RatePerMin`
| Input Options     | `Rate Per Minute`<br>`Rate Per Processing`<br>`No Rate Check`| `RatePerMin`<br>`RatePerProcessing`<br>`RateOff`


## Description

This parameter defines if the maximum rate evaluation is performed using the difference of the last two readings 
(`Rate Per Processing` == rate / processing interval) or using the difference of the last two readings normalized 
to a minute (`Rate Per Minute`). Setting it to `No Rate Check` no maximum rate evaluation will be performed.<br><br> 


!!! Note
    The rate will be checked against positive and negative rate deviation.

!!! Note
    This parameter can to be set for each number sequence separately.
    Use the drop down to choose the respective number sequence. 
    A number sequence is a group of single digits and / or analog counter defined in digit and / or anlog ROI configuration screen.

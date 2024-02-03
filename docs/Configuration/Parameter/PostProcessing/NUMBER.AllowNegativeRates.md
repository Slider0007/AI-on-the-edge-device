# Parameter: Allow Negative Rates

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Allow Negative Rates | [NUMBER SEQUENCE].AllowNegativeRates
| Default Value     | `false`             | `false`
| Input Options     | `false`<br>`true`   | `false`<br>`true` 


!!! Warning
    This is an **Expert Parameter**! Only change it if you understand what it does!  


## Description

Allow decreasing values (backwards counting).


!!! Note
    For most use cases this option should be set to `false` e.g. for water or gas meters (-> plausiblity check can be performed to avoid negative rates).
    But for some use cases like for e.g. pressure sensors negative rates a acepted.


!!! Note
    This parameter can to be set for each number sequence separately. Use the drop down to choose the respective number sequence. 
    A number sequence is a group of single digits and / or analog counter defined in digit and / or anlog ROI configuration screen.

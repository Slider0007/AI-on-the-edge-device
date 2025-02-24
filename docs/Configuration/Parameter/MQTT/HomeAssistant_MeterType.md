# Parameter: Meter Type

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Meter Type          | metertype
| Default Value     | `Water (Value: m³, Rate: m³/h)`  | `1`
| Input Options     | `Generic (No Units)`<br>`Water (Value: m³, Rate: m³/h)`<br>`Water (Value: L, Rate: L/h*)`<br>`Water (Value: L, Rate: L/min)`<br>`Water (Value: gal, Rate: gal/h*)`<br>`Water (Value: gal, Rate: gal/min)`<br>`Water (Value: ft³, Rate: ft³/min)`<br>`Gas (Value: m³, Rate: m³/h)`<br>`Gas (Value: ft³, Rate: ft³/min)`<br>`Energy (Value: Wh, Rate: W)`<br>`Energy (Value: kWh, Rate: kW)`<br>`Energy (Value: MWh, Rate: MW)`<br>`Energy (Value: GJ, Rate: GJ/h*)`<br>`Temperature (Value: °C, Rate: K/min)`<br>`Temperature (Value: °F, Rate: K/min)`<br>`Pressure (Value: bar, Rate: bar/min)`<br>`Pressure (Value: psi, Rate: psi/min)` | `0` .. `16`


## Description

Select the meter type to configure the suitable icons / units in Home Assistant.


!!! Note
    Using `Watermeter` Home Assistant 2022.11 or newer is mandatory.<br>
    * Rate unit is not officially supported in Home Assistant: Use device class `None`


!!! Note
    Please make sure that the selected meter type matches the dimension provided by the meter.
    Eg. if your meter provides `m³`, set this parameter to `m³`.
    Any necessary conversion needs to be done using the `Decimal Shift´ parameter.
    

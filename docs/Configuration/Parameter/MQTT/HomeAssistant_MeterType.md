# Parameter: Home Assistant Meter Type

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Home Assistant Meter Type | metertype
| Default Value     | `Watermeter (Value: m³, Rate: m³/h)`  | `1`
| Input Options     | `None (no Units)`<br>`Watermeter (Value: m³, Rate: m³/h)`<br>`Watermeter (Value: l, Rate: l/h)`<br>`Watermeter (Value: gal, Rate: gal/h)`<br>`Watermeter (Value: ft³, Rate: ft³/m)`<br>`Gasmeter (Value: m³, Rate: m³/h)`<br>`Gasmeter (Value: ft³, Rate: ft³/m)`<br>`Energymeter (Value: Wh, Rate: W)`<br>`Energymeter (Value: kWh, Rate: kW)`<br>`Energymeter (Value: MWh, Rate: MW)`<br>`Energymeter (Value: GJ, Rate: GJ/h)` | `0` .. `10`


## Description

Select the meter type so the sensors have the right units in Homeassistant.


!!! Note
    Using `Watermeter` Home Assistant 2022.11 or newer is mandatory!


!!! Note
    Please make sure that the selected meter type matches the dimension provided by the meter.
    Eg. if your meter provides `m³`, set this parameter to `m³`.
    Any necessary conversion needs to be done using the `Decimal Shift´ parameter.
    

# Parameter: Meter Type

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Meter Type          | MeterType
| Default Value     | `Other (no Units)`  | `other`
| Input Options     | `Other (no Units)`<br>`Watermeter (Value: m³, Rate: m³/h)`<br>`Watermeter (Value: l, Rate: l/h)`<br>`Watermeter (Value: gal, Rate: gal/h)`<br>`Watermeter (Value: ft³, Rate: ft³/m)`<br>`Gasmeter (Value: m³, Rate: m³/h)`<br>`Gasmeter (Value: ft³, Rate: ft³/m)`<br>`Energymeter (Value: Wh, Rate: W)`<br>`Energymeter (Value: kWh, Rate: kW)`<br>`Energymeter (Value: MWh, Rate: MW)`<br>`Energymeter (Value: GJ, Rate: GJ/h)` | `other`<br>`water_m3`<br>`water_l`<br>`water_gal`<br>`water_ft3`<br>`gas_m3`<br>`gas_ft3`<br>`energy_wh`<br>`energy_kwh`<br>`energy_mwh`<br>`energy_gj`


## Description

Select the meter type so the sensors have the right units in Homeassistant.


!!! Note
    For `Watermeter` you need to have Homeassistant 2022.11 or newer!


!!! Note
    Please make sure that the selected meter type matches the dimension provided by the meter.
    Eg. if your meter provides `m³`, set this parameter to `m³`.
    Any necessary conversion needs to be done using the `Decimal Shift´ parameter.
    

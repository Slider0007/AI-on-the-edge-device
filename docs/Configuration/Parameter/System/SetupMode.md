# Parameter: Setup Mode

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | N/A                 | SetupMode
| Default Value     | N/A                 | `true`
| Input Options     | N/A                 | `false`<br>`true` 


## Description

If this parameter is set to `true`, device is booting directly into WebUI setup procedure.
After completion/abortion of the setup procedure the parameter gets automatically set to `false`.
If the parameter is set to `false` the device boots directly into the regular web interface.


!!! Note
    Usually this parameter is only set to `true` after device is set up completely from sctrach.
    This parameter can only be changed by modifying directly in config.ini.


!!! Tip
    If the device is not switching to the respective WebUI view, most likely it's related to browser cache issues.
    Please force reload (e.g. with CTRL +F5) the WebUI or manually clean the browser cache.

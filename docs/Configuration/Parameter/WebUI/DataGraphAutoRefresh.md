# Parameter: Auto Refresh

|                   | WebUI               | Config.ini
|:---               |:---                 |:----
| Parameter Name    | Auto Refresh        | DataGraphAutoRefresh
| Default Value     | `false`             | `false`
| Input Options     | `false`<br>`true`   | `false`<br>`true` 


## Description

Enable or disable the automatic data refresh of WebUI page `Data Graph`. The chart gets updated in 
the defined refresh interval.


!!! Tip
    This can be temporarily overridden on the 'Data Graph' WebUI page, e.g. to temporarily switch off 
    the automatic refresh. Manual refreshing is also possible.


!!! Note
    The refresh interval needs to be configured with parameter `Auto Refresh Time` in this subsection.

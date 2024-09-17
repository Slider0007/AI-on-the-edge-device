# Parameter: Page Auto Refresh

|                   | WebUI               | REST API
|:---               |:---                 |:----
| Parameter Name    | Auto Refresh        | enabled
| Default Value     | `Disabled`          | `false`
| Input Options     | `Disabled`<br>`Enabled` | `false`<br>`true` 


## Description

Enable or disable the automatic data refresh of WebUI page `Data Graph`. The chart gets updated in 
the defined refresh interval.


!!! Tip
    This can be temporarily overridden on the 'Data Graph' WebUI page, e.g. to temporarily switch off 
    the automatic refresh. Manual refreshing is also possible.


!!! Note
    The refresh interval needs to be configured with parameter `Refresh Time` in this subsection.
